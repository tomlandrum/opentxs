// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"    // IWYU pragma: associated
#include "1_Internal.hpp"  // IWYU pragma: associated
#include "internal/api/client/blockchain/Blockchain.hpp"  // IWYU pragma: associated

#if OT_BLOCKCHAIN
#include <boost/container/flat_map.hpp>
#endif  // OT_BLOCKCHAIN

#if OT_BLOCKCHAIN
#include "internal/blockchain/Params.hpp"
#endif  // OT_BLOCKCHAIN
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Hash.hpp"
#if OT_BLOCKCHAIN
#include "opentxs/blockchain/Blockchain.hpp"
#endif  // OT_BLOCKCHAIN
#include "opentxs/core/Log.hpp"
#include "opentxs/crypto/HashType.hpp"

namespace opentxs
{
auto blockchain_thread_item_id(
    const api::Crypto& crypto,
    const opentxs::blockchain::Type chain,
    const Data& txid) noexcept -> OTIdentifier
{
    auto preimage = std::string{};
    const auto hashed = crypto.Hash().HMAC(
        crypto::HashType::Sha256,
        ReadView{reinterpret_cast<const char*>(&chain), sizeof(chain)},
        txid.Bytes(),
        writer(preimage));

    OT_ASSERT(hashed);

    auto id = Identifier::Factory();
    id->CalculateDigest(preimage);

    return id;
}
}  // namespace opentxs

#if OT_BLOCKCHAIN
namespace opentxs::api::client::blockchain
{
constexpr auto sync_map_ = [] {
    constexpr auto offset{65536};
    auto map = boost::container::flat_map<Chain, SyncTableData>{};

    for (const auto& chain : opentxs::blockchain::DefinedChains()) {
        auto& [table, name] = map[chain];
        table = offset + static_cast<int>(chain);
        name = opentxs::blockchain::params::Data::Chains()
                   .at(chain)
                   .display_string_;
    }

    return map;
};

auto ChainToSyncTable(const Chain chain) noexcept(false) -> int
{
    static const auto map = sync_map_();

    return map.at(chain).first;
}

auto SyncTables() noexcept -> const std::vector<SyncTableData>&
{
    static const auto map = [] {
        auto output = std::vector<SyncTableData>{};

        for (const auto& [key, value] : sync_map_()) {
            output.emplace_back(value);
        }

        return output;
    }();

    return map;
}
}  // namespace opentxs::api::client::blockchain
#endif  // OT_BLOCKCHAIN
