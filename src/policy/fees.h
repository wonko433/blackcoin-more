// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_POLICYESTIMATOR_H
#define BITCOIN_POLICYESTIMATOR_H

#include "amount.h"
#include "uint256.h"
#include "random.h"

#include <map>
#include <string>
#include <vector>

class CFeeRate;

class FeeFilterRounder
{
private:
    static constexpr double MAX_FILTER_FEERATE = 1e7;
    /** FEE_FILTER_SPACING is just used to provide some quantization of fee
     * filter results.  Historically it reused FEE_SPACING, but it is completely
     * unrelated, and was made a separate constant so the two concepts are not
     * tied together */
    static constexpr double FEE_FILTER_SPACING = 1.1;

public:
    /** Create new FeeFilterRounder */
    FeeFilterRounder(const CFeeRate& minIncrementalFee);

    /** Quantize a minimum fee for privacy purpose before broadcast **/
    CAmount round(CAmount currentMinFee);

private:
    std::set<double> feeset;
    FastRandomContext insecure_rand;
};

#endif /*BITCOIN_POLICYESTIMATOR_H */
