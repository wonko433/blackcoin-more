// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <outputtype.h>

#include <pubkey.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/standard.h>

#include <assert.h>
#include <string>

static const std::string OUTPUT_TYPE_STRING_LEGACY = "legacy";
static const std::string OUTPUT_TYPE_STRING_BECH32 = "bech32";

bool ParseOutputType(const std::string& type, OutputType& output_type)
{
    if (type == OUTPUT_TYPE_STRING_LEGACY) {
        output_type = OutputType::LEGACY;
        return true;
    } else if (type == OUTPUT_TYPE_STRING_BECH32) {
        output_type = OutputType::BECH32;
        return true;
    }
    return false;
}

const std::string& FormatOutputType(OutputType type)
{
    switch (type) {
    case OutputType::LEGACY: return OUTPUT_TYPE_STRING_LEGACY;
    case OutputType::BECH32: return OUTPUT_TYPE_STRING_BECH32;
    default: assert(false);
    }
}

CTxDestination GetDestinationForKey(const CPubKey& key, OutputType type)
{
    switch (type) {
    case OutputType::LEGACY: return PKHash(key);
    case OutputType::BECH32: {
        if (!key.IsCompressed()) return PKHash(key);
    }
    default: assert(false);
    }
}

std::vector<CTxDestination> GetAllDestinationsForKey(const CPubKey& key)
{
    PKHash keyid(key);
    if (key.IsCompressed()) {
        // Do nothing
    } else {
        return std::vector<CTxDestination>{std::move(keyid)};
    }

    return std::vector<CTxDestination>{std::move(keyid)};
}

CTxDestination AddAndGetDestinationForScript(FillableSigningProvider& keystore, const CScript& script, OutputType type)
{
    // Add script to keystore
    keystore.AddCScript(script);
    // Note that scripts over 520 bytes are not yet supported.
    switch (type) {
    case OutputType::LEGACY:
        return ScriptHash(script);
    case OutputType::BECH32: {
        return ScriptHash(script);
    }
    default: assert(false);
    }
}
