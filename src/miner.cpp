// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <miner.h>

#include <amount.h>
#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/consensus.h>
#include <consensus/tx_verify.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <hash.h>
#include <net.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <pos.h>
#include <pow.h>
#include <primitives/transaction.h>
#include <script/standard.h>
#include <timedata.h>
#include <util.h>
#include <utilmoneystr.h>
#include <validationinterface.h>
#include <wallet/wallet.h>

#include <algorithm>
#include <memory>
#include <queue>
#include <utility>

// Unconfirmed transactions in the memory pool often depend on other
// transactions in the memory pool. When we select transactions from the
// pool, we select by highest fee rate of a transaction combined with all
// its ancestors.

uint64_t nLastBlockTx = 0;
uint64_t nLastBlockSize = 0;
int64_t nLastCoinStakeSearchInterval = 0;
unsigned int nMinerSleep = 500;

int64_t UpdateTime(CBlock* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    int64_t nOldTime = pblock->nTime;
    int64_t nNewTime = std::max(pindexPrev->GetPastTimeLimit()+1, GetAdjustedTime());

    if (nOldTime < nNewTime)
        pblock->nTime = nNewTime;

    // Updating time can change work required on testnet:
    if (consensusParams.fPowAllowMinDifficultyBlocks)
        pblock->nBits = GetNextTargetRequired(pindexPrev, pblock, consensusParams, pblock->IsProofOfStake());

    return nNewTime - nOldTime;
}

int64_t GetMaxTransactionTime(CBlock* pblock)
{
    int64_t maxTransactionTime = 0;
    for (std::vector<CTransactionRef>::const_iterator it(pblock->vtx.begin()); it != pblock->vtx.end(); ++it)
        maxTransactionTime = std::max(maxTransactionTime, (int64_t)it->get()->nTime);
    return maxTransactionTime;
}

BlockAssembler::Options::Options() {
    blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);
    nBlockMaxSize = DEFAULT_BLOCK_MAX_SIZE;
}

BlockAssembler::BlockAssembler(const CChainParams& params, const Options& options) : chainparams(params)
{
    blockMinFeeRate = options.blockMinFeeRate;
    // Limit size to between 1K and MAX_BLOCK_SIZE-1K for sanity:
    nBlockMaxSize = std::max<size_t>(1000, std::min<size_t>(MAX_BLOCK_SIZE - 1000, options.nBlockMaxSize));
}

static BlockAssembler::Options DefaultOptions()
{
    // Block resource limits
    // If -blockmaxsize is not given, limit to DEFAULT_BLOCK_MAX_SIZE
    BlockAssembler::Options options;
    options.nBlockMaxSize = gArgs.GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
    if (gArgs.IsArgSet("-blockmintxfee")) {
        CAmount n = 0;
        ParseMoney(gArgs.GetArg("-blockmintxfee", ""), n);
        options.blockMinFeeRate = CFeeRate(n);
    } else {
        options.blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);
    }
    return options;
}

BlockAssembler::BlockAssembler(const CChainParams& params) : BlockAssembler(params, DefaultOptions()) {}

void BlockAssembler::resetBlock()
{
    inBlock.clear();

    // Reserve space for coinbase tx
    nBlockSize = 1000;
    nBlockSigOps = 100;

    // These counters do not include coinbase tx
    nBlockTx = 0;
    nFees = 0;
}

std::unique_ptr<CBlockTemplate> BlockAssembler::CreateNewBlock(const CScript& scriptPubKeyIn, int64_t* pFees, bool fProofOfStake)
{
    int64_t nTimeStart = GetTimeMicros();

    resetBlock();

    pblocktemplate.reset(new CBlockTemplate());

    if(!pblocktemplate.get())
        return nullptr;
    CBlock *pblock = &pblocktemplate->block; // pointer for convenience

    // Add dummy coinbase tx as first transaction
    pblock->vtx.emplace_back();
    pblocktemplate->vTxFees.push_back(-1); // updated at end
    pblocktemplate->vTxSigOpsCount.push_back(-1); // updated at end

    LOCK2(cs_main, mempool.cs);
    CBlockIndex* pindexPrev = chainActive.Tip();
    assert(pindexPrev != nullptr);
    nHeight = pindexPrev->nHeight + 1;

    pblock->nVersion = ComputeBlockVersion(pindexPrev, chainparams.GetConsensus());
    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (chainparams.MineBlocksOnDemand())
        pblock->nVersion = gArgs.GetArg("-blockversion", pblock->nVersion);

    pblock->nTime = GetAdjustedTime();
    const int64_t nGetPastTimeLimit = pindexPrev->GetPastTimeLimit();

    nLockTimeCutoff = (STANDARD_LOCKTIME_VERIFY_FLAGS & LOCKTIME_MEDIAN_TIME_PAST)
                       ? nGetPastTimeLimit
                       : pblock->GetBlockTime();

    int nPackagesSelected = 0;
    int nDescendantsUpdated = 0;
    addPackageTxs(nPackagesSelected, nDescendantsUpdated);

    int64_t nTime1 = GetTimeMicros();

    nLastBlockTx = nBlockTx;
    nLastBlockSize = nBlockSize;

    // Create coinbase transaction.
    CMutableTransaction coinbaseTx;
    coinbaseTx.nTime = pblock->nTime;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vout.resize(1);
    if (fProofOfStake) {
        // Make the coinbase tx empty in case of proof of stake
        coinbaseTx.vout[0].SetEmpty();
    }
    else {
        coinbaseTx.vout[0].scriptPubKey = scriptPubKeyIn;
        coinbaseTx.vout[0].nValue = nFees + GetProofOfWorkSubsidy();
    }
    coinbaseTx.vin[0].scriptSig = CScript() << nHeight << OP_0;
    pblock->vtx[0] = MakeTransactionRef(std::move(coinbaseTx));
    pblocktemplate->vTxFees[0] = -nFees;

    LogPrintf("CreateNewBlock(): block size: %u txs: %u fees: %ld sigops %d\n", nBlockSize, nBlockTx, nFees, nBlockSigOps);

    if (pFees)
        *pFees = nFees;

    // Fill in header
    pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
    pblock->nTime = max(pindexPrev->GetPastTimeLimit()+1, GetMaxTransactionTime(pblock));
    if (!fProofOfStake)
        UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
    pblock->nBits          = GetNextTargetRequired(pindexPrev, pblock, chainparams.GetConsensus(), fProofOfStake);
    pblock->nNonce         = 0;
    pblocktemplate->vTxSigOpsCount[0] = GetSigOpCountWithoutP2SH(*pblock->vtx[0]);

    CValidationState state;
    if (!fProofOfStake && !TestBlockValidity(state, chainparams, *pblock, pindexPrev, false, false, true)) {
        throw std::runtime_error(strprintf("%s: TestBlockValidity failed: %s", __func__, FormatStateMessage(state)));
    }
    int64_t nTime2 = GetTimeMicros();

    LogPrint(BCLog::BENCH, "CreateNewBlock() packages: %.2fms (%d packages, %d updated descendants), validity: %.2fms (total %.2fms)\n", 0.001 * (nTime1 - nTimeStart), nPackagesSelected, nDescendantsUpdated, 0.001 * (nTime2 - nTime1), 0.001 * (nTime2 - nTimeStart));

    return std::move(pblocktemplate);
}

void BlockAssembler::onlyUnconfirmed(CTxMemPool::setEntries& testSet)
{
    for (CTxMemPool::setEntries::iterator iit = testSet.begin(); iit != testSet.end(); ) {
        // Only test txs not already in the block
        if (inBlock.count(*iit)) {
            testSet.erase(iit++);
        }
        else {
            iit++;
        }
    }
}

bool BlockAssembler::TestPackage(uint64_t packageSize, int64_t packageSigOps) const
{
    auto blockSizeWithPackage = nBlockSize + packageSize;
    if (blockSizeWithPackage >= DEFAULT_BLOCK_MAX_SIZE)
        return false;
    if (nBlockSigOps + packageSigOps >= MAX_BLOCK_SIGOPS)
        return false;
    return true;
}

// Perform transaction-level checks before adding to block:
// - transaction finality (locktime)
// - serialized size (in case -blockmaxsize is in use)
bool BlockAssembler::TestPackageTransactions(const CTxMemPool::setEntries& package)
{
    uint64_t nPotentialBlockSize = nBlockSize;
    for (CTxMemPool::txiter it : package) {
        if (!IsFinalTx(it->GetTx(), nHeight, nLockTimeCutoff))
            return false;
        uint64_t nTxSize = ::GetSerializeSize(it->GetTx(), SER_NETWORK, PROTOCOL_VERSION);
        if (nPotentialBlockSize + nTxSize >= nBlockMaxSize)
            return false;
        nPotentialBlockSize += nTxSize;
    }
    return true;
}

void BlockAssembler::AddToBlock(CTxMemPool::txiter iter)
{
    CBlock *pblock = &pblocktemplate->block;

    pblock->vtx.emplace_back(iter->GetSharedTx());
    pblocktemplate->vTxFees.push_back(iter->GetFee());
    pblocktemplate->vTxSigOpsCount.push_back(iter->GetSigOpCount());
    nBlockSize += iter->GetTxSize();
    ++nBlockTx;
    nBlockSigOps += iter->GetSigOpCount();
    nFees += iter->GetFee();
    inBlock.insert(iter);

    bool fPrintPriority = gArgs.GetBoolArg("-printpriority", DEFAULT_PRINTPRIORITY);
    if (fPrintPriority) {
        LogPrintf("fee %s txid %s\n",
                  CFeeRate(iter->GetModifiedFee(), iter->GetTxSize()).ToString(),
                  iter->GetTx().GetHash().ToString());
    }
}

int BlockAssembler::UpdatePackagesForAdded(const CTxMemPool::setEntries& alreadyAdded,
        indexed_modified_transaction_set &mapModifiedTx)
{
    int nDescendantsUpdated = 0;
    for (CTxMemPool::txiter it : alreadyAdded) {
        CTxMemPool::setEntries descendants;
        mempool.CalculateDescendants(it, descendants);
        // Insert all descendants (not yet in block) into the modified set
        for (CTxMemPool::txiter desc : descendants) {
            if (alreadyAdded.count(desc))
                continue;
            ++nDescendantsUpdated;
            modtxiter mit = mapModifiedTx.find(desc);
            if (mit == mapModifiedTx.end()) {
                CTxMemPoolModifiedEntry modEntry(desc);
                modEntry.nSizeWithAncestors -= it->GetTxSize();
                modEntry.nModFeesWithAncestors -= it->GetModifiedFee();
                modEntry.nSigOpCountWithAncestors -= it->GetSigOpCount();
                mapModifiedTx.insert(modEntry);
            } else {
                mapModifiedTx.modify(mit, update_for_parent_inclusion(it));
            }
        }
    }
    return nDescendantsUpdated;
}

// Skip entries in mapTx that are already in a block or are present
// in mapModifiedTx (which implies that the mapTx ancestor state is
// stale due to ancestor inclusion in the block)
// Also skip transactions that we've already failed to add. This can happen if
// we consider a transaction in mapModifiedTx and it fails: we can then
// potentially consider it again while walking mapTx.  It's currently
// guaranteed to fail again, but as a belt-and-suspenders check we put it in
// failedTx and avoid re-evaluation, since the re-evaluation would be using
// cached size/sigops/fee values that are not actually correct.
bool BlockAssembler::SkipMapTxEntry(CTxMemPool::txiter it, indexed_modified_transaction_set &mapModifiedTx, CTxMemPool::setEntries &failedTx)
{
    assert (it != mempool.mapTx.end());
    return mapModifiedTx.count(it) || inBlock.count(it) || failedTx.count(it);
}

void BlockAssembler::SortForBlock(const CTxMemPool::setEntries& package, std::vector<CTxMemPool::txiter>& sortedEntries)
{
    // Sort package by ancestor count
    // If a transaction A depends on transaction B, then A's ancestor count
    // must be greater than B's.  So this is sufficient to validly order the
    // transactions for block inclusion.
    sortedEntries.clear();
    sortedEntries.insert(sortedEntries.begin(), package.begin(), package.end());
    std::sort(sortedEntries.begin(), sortedEntries.end(), CompareTxIterByAncestorCount());
}

// This transaction selection algorithm orders the mempool based
// on feerate of a transaction including all unconfirmed ancestors.
// Since we don't remove transactions from the mempool as we select them
// for block inclusion, we need an alternate method of updating the feerate
// of a transaction with its not-yet-selected ancestors as we go.
// This is accomplished by walking the in-mempool descendants of selected
// transactions and storing a temporary modified state in mapModifiedTxs.
// Each time through the loop, we compare the best transaction in
// mapModifiedTxs with the next transaction in the mempool to decide what
// transaction package to work on next.
void BlockAssembler::addPackageTxs(int &nPackagesSelected, int &nDescendantsUpdated)
{
    // mapModifiedTx will store sorted packages after they are modified
    // because some of their txs are already in the block
    indexed_modified_transaction_set mapModifiedTx;
    // Keep track of entries that failed inclusion, to avoid duplicate work
    CTxMemPool::setEntries failedTx;

    // Start by adding all descendants of previously added txs to mapModifiedTx
    // and modifying them for their already included ancestors
    UpdatePackagesForAdded(inBlock, mapModifiedTx);

    CTxMemPool::indexed_transaction_set::index<ancestor_score>::type::iterator mi = mempool.mapTx.get<ancestor_score>().begin();
    CTxMemPool::txiter iter;

    // Limit the number of attempts to add transactions to the block when it is
    // close to full; this is just a simple heuristic to finish quickly if the
    // mempool has a lot of entries.
    const int64_t MAX_CONSECUTIVE_FAILURES = 1000;
    int64_t nConsecutiveFailed = 0;

    while (mi != mempool.mapTx.get<ancestor_score>().end() || !mapModifiedTx.empty())
    {
        // First try to find a new transaction in mapTx to evaluate.
        if (mi != mempool.mapTx.get<ancestor_score>().end() &&
                SkipMapTxEntry(mempool.mapTx.project<0>(mi), mapModifiedTx, failedTx)) {
            ++mi;
            continue;
        }

        // Now that mi is not stale, determine which transaction to evaluate:
        // the next entry from mapTx, or the best from mapModifiedTx?
        bool fUsingModified = false;

        modtxscoreiter modit = mapModifiedTx.get<ancestor_score>().begin();
        if (mi == mempool.mapTx.get<ancestor_score>().end()) {
            // We're out of entries in mapTx; use the entry from mapModifiedTx
            iter = modit->iter;
            fUsingModified = true;
        } else {
            // Try to compare the mapTx entry to the mapModifiedTx entry
            iter = mempool.mapTx.project<0>(mi);
            if (modit != mapModifiedTx.get<ancestor_score>().end() &&
                    CompareTxMemPoolEntryByAncestorFee()(*modit, CTxMemPoolModifiedEntry(iter))) {
                // The best entry in mapModifiedTx has higher score
                // than the one from mapTx.
                // Switch which transaction (package) to consider
                iter = modit->iter;
                fUsingModified = true;
            } else {
                // Either no entry in mapModifiedTx, or it's worse than mapTx.
                // Increment mi for the next loop iteration.
                ++mi;
            }
        }

        // We skip mapTx entries that are inBlock, and mapModifiedTx shouldn't
        // contain anything that is inBlock.
        assert(!inBlock.count(iter));

        uint64_t packageSize = iter->GetSizeWithAncestors();
        CAmount packageFees = iter->GetModFeesWithAncestors();
        int64_t packageSigOps = iter->GetSigOpCountWithAncestors();
        if (fUsingModified) {
            packageSize = modit->nSizeWithAncestors;
            packageFees = modit->nModFeesWithAncestors;
            packageSigOps = modit->nSigOpCountWithAncestors;
        }

        if (packageFees < blockMinFeeRate.GetFee(packageSize)) {
            // Everything else we might consider has a lower fee rate
            return;
        }

        if (!TestPackage(packageSize, packageSigOps)) {
            if (fUsingModified) {
                // Since we always look at the best entry in mapModifiedTx,
                // we must erase failed entries so that we can consider the
                // next best entry on the next loop iteration
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }

            ++nConsecutiveFailed;

            if (nConsecutiveFailed > MAX_CONSECUTIVE_FAILURES && nBlockSize >
                    nBlockMaxSize - 4000) {
                // Give up if we're close to full and haven't succeeded in a while
                break;
            }
            continue;
        }

        CTxMemPool::setEntries ancestors;
        uint64_t nNoLimit = std::numeric_limits<uint64_t>::max();
        std::string dummy;
        mempool.CalculateMemPoolAncestors(*iter, ancestors, nNoLimit, nNoLimit, nNoLimit, nNoLimit, dummy, false);

        onlyUnconfirmed(ancestors);
        ancestors.insert(iter);

        // Test if all tx's are Final
        if (!TestPackageTransactions(ancestors)) {
            if (fUsingModified) {
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }
            continue;
        }

        // This transaction will make it in; reset the failed counter.
        nConsecutiveFailed = 0;

        // Package can be added. Sort the entries in a valid order.
        std::vector<CTxMemPool::txiter> sortedEntries;
        SortForBlock(ancestors, sortedEntries);

        for (size_t i=0; i<sortedEntries.size(); ++i) {
            AddToBlock(sortedEntries[i]);
            // Erase from the modified set, if present
            mapModifiedTx.erase(sortedEntries[i]);
        }

        ++nPackagesSelected;

        // Update transactions that depend on each of these
        nDescendantsUpdated += UpdatePackagesForAdded(ancestors, mapModifiedTx);
    }
}

void IncrementExtraNonce(CBlock* pblock, const CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock)
    {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;
    unsigned int nHeight = pindexPrev->nHeight+1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(*pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(nExtraNonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);

    pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
}

bool CheckStake(std::shared_ptr<CBlock> pblock, CWallet& wallet, const CChainParams& chainparams)
{
    uint256 hashBlock = pblock->GetHash();

    if (!pblock->IsProofOfStake())
        return error("CheckStake() : %s is not a proof-of-stake block", hashBlock.GetHex());

    // verify hash target and signature of coinstake tx
    CValidationState state;
    if (!CheckProofOfStake(mapBlockIndex[pblock->hashPrevBlock], *pblock->vtx[1], pblock->nBits, state, *pcoinsTip))
        return error("CheckStake() : proof-of-stake checking failed");

    //// debug print
    LogPrint(BCLog::COINSTAKE, "%s\n", pblock->ToString());
    LogPrint(BCLog::COINSTAKE, "out %s\n", FormatMoney(pblock->vtx[1]->GetValueOut()));

    // Found a solution
    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != chainActive.Tip()->GetBlockHash())
            return error("CheckStake() : generated block is stale");

        for (const CTxIn& vin : pblock->vtx[1]->vin) {
            if (wallet.IsSpent(vin.prevout.hash, vin.prevout.n)) {
                return error("CheckStake() : generated block became invalid due to stake UTXO being spent");
            }
        }

        // Process this block the same as if we had received it from another node
        std::shared_ptr<const CBlock> staked_pblock =
                    std::make_shared<const CBlock>(*pblock);
        if (!ProcessNewBlock(chainparams, staked_pblock, true, NULL))
            return error("CheckStake() : ProcessNewBlock, block not accepted");
    }

    return true;
}

void ThreadStakeMiner(CWallet *pwallet, const CChainParams& chainparams)
{
    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    LogPrintf("Staking started\n");

    // Make this thread recognisable as the mining thread
    RenameThread("blackcoin-miner");

    CReserveKey reservekey(pwallet);

    try {
        bool fTryToSync = true;
        bool regtestMode = Params().GetConsensus().fPoSNoRetargeting;
        if (regtestMode) {
            nMinerSleep = 30000; //limit regtest to 30s, otherwise it'll create 2 blocks per second
        }

        while (true)
        {
            while (pwallet->IsLocked())
            {
                pwallet->m_last_coin_stake_search_interval = 0;
                MilliSleep(10000);
            }

            if (!regtestMode) {
                while (g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL) == 0 || IsInitialBlockDownload())
                {
                    pwallet->m_last_coin_stake_search_interval = 0;
                    fTryToSync = true;
                    MilliSleep(1000);
                }
                if (fTryToSync)
                {
                    fTryToSync = false;
                    if (g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL) < 3 || pindexBestHeader->GetBlockTime() < GetTime() - 10 * 60)
                    {
                        MilliSleep(60000);
                        continue;
                    }
                }
            }

            //
            // Create new block
            //
            if (pwallet->HaveAvailableCoinsForStaking())
            {
                int64_t nFees = 0;
                // First just create an empty block. No need to process transactions until we know we can create a block
                std::unique_ptr<CBlockTemplate> pblocktemplate(BlockAssembler(Params()).CreateNewBlock(reservekey.reserveScript, &nFees, true));
                if (!pblocktemplate.get())
                    return;

                std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>(pblocktemplate->block);
                pblock->nFlags = CBlockIndex::BLOCK_PROOF_OF_STAKE;

                if (pwallet->m_last_coin_stake_search_time == 0)
                    pwallet->m_last_coin_stake_search_time = GetAdjustedTime(); // startup timestamp

                int64_t nSearchTime = GetAdjustedTime();
                nSearchTime &= ~Params().GetConsensus().nStakeTimestampMask;

                if (nSearchTime > pwallet->m_last_coin_stake_search_time) 
				{
					// Trying to sign a block
					if (SignBlock(pblock, *pwallet, nFees, nSearchTime))
					{
						// increase priority
						SetThreadPriority(THREAD_PRIORITY_ABOVE_NORMAL);
						// Sign the full block
						CheckStake(pblock, *pwallet, chainparams);
						// return back to low priority
						SetThreadPriority(THREAD_PRIORITY_LOWEST);
						MilliSleep(500);
					}
				pwallet->m_last_coin_stake_search_interval = nSearchTime - pwallet->m_last_coin_stake_search_time;
				pwallet->m_last_coin_stake_search_time = nSearchTime;
				}
            }
                MilliSleep(nMinerSleep);
        }
    }
    catch (const boost::thread_interrupted&)
    {
        LogPrintf("Staking stopped\n");
        throw;
    }
    catch (const std::runtime_error &e)
    {
        LogPrintf("ThreadStakeMiner(): runtime error: %s\n", e.what());
        return;
    }
}

// novacoin: attempt to generate suitable proof-of-stake
bool SignBlock(std::shared_ptr<CBlock> pblock, CWallet& wallet, int64_t& nFees, int64_t nTime)
{
    CWallet* const pwallet = wallet.get();
    // if we are trying to sign
    // something except proof-of-stake block template
    if (!pblock->vtx[0]->vout[0].IsEmpty()){
        LogPrintf("SignBlock(): Trying to sign something except proof-of-stake block!\n");
        return false;
    }

    // if we are trying to sign
    // a complete proof-of-stake block
    if (pblock->IsProofOfStake()) {
        return true;
    }

    CKey key;
    CMutableTransaction txCoinBase(*pblock->vtx[0]);
    CMutableTransaction txCoinStake;
    txCoinStake.nTime = nTime;

    if (wallet.CreateCoinStake(wallet, pblock->nBits, 1, nFees, txCoinStake, key))
    {
        if (txCoinStake.nTime >= pindexBestHeader->GetPastTimeLimit()+1)
        {
            // make sure coinstake would meet timestamp protocol
            // as it would be the same as the block timestamp
            txCoinBase.nTime = pblock->nTime = txCoinStake.nTime;
            pblock->vtx[0] = MakeTransactionRef(std::move(txCoinBase));

            // we have to make sure that we have no future timestamps in
            // our transactions set
            for (std::vector<CTransactionRef>::iterator it = pblock->vtx.begin(); it != pblock->vtx.end();)
                if (it->get()->nTime > pblock->nTime) { it = pblock->vtx.erase(it); } else { ++it; }

            pblock->vtx.insert(pblock->vtx.begin() + 1, MakeTransactionRef(txCoinStake));

            pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);

            // append a signature to our block
            return key.Sign(pblock->GetHash(), pblock->vchBlockSig);
        }
    }

return false;
}

void StakeCoins(bool fStake, CWallet *pwallet, CConnman* connman, boost::thread_group*& stakeThread)
{
    if (stakeThread != nullptr)
    {
        stakeThread->interrupt_all();
        delete stakeThread;
        stakeThread = nullptr;
    }

    if (fStake)
    {
        stakeThread = new boost::thread_group();
        stakeThread->create_thread(boost::bind(&ThreadStakeMiner, pwallet, connman));
    }
}
