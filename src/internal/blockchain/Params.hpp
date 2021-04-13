// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <boost/container/flat_map.hpp>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "display/Definition.hpp"
#if OT_BLOCKCHAIN
#include "internal/blockchain/p2p/bitcoin/Bitcoin.hpp"
#endif  // OT_BLOCKCHAIN
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/client/blockchain/AddressStyle.hpp"
#include "opentxs/api/client/blockchain/Types.hpp"
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/blockchain/FilterType.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/blockchain/p2p/Types.hpp"
#include "opentxs/crypto/Bip44Type.hpp"
#include "opentxs/crypto/Types.hpp"

namespace opentxs::blockchain::params
{
struct Data {
    using ChainData = boost::container::flat_map<blockchain::Type, Data>;
#if OT_BLOCKCHAIN
    using FilterData = boost::container::flat_map<
        blockchain::Type,
        boost::container::
            flat_map<filter::Type, std::pair<std::string, std::string>>>;
    using FilterTypes = std::map<Type, std::map<filter::Type, std::uint8_t>>;
    using ServiceBits = std::
        map<blockchain::Type, std::map<p2p::bitcoin::Service, p2p::Service>>;
#endif  // OT_BLOCKCHAIN
    using Style = api::client::blockchain::AddressStyle;
    using ScriptMap = boost::container::flat_map<Style, bool>;
    using StylePref = std::vector<std::pair<Style, std::string>>;

    struct Checkpoint {
        block::Height height_{};
        std::string block_hash_{};
        std::string previous_block_hash_{};
        std::string filter_header_{};
    };

    bool supported_{};
    bool testnet_{};
    contact::ContactItemType itemtype_{};
    Bip44Type bip44_{};
    std::string display_string_{};
    std::string display_ticker_{};
    std::int32_t nBits_{};
    std::string genesis_header_hex_{};
    std::string genesis_hash_hex_{};
    std::string genesis_block_hex_{};
    Checkpoint checkpoint_{};
    filter::Type default_filter_type_{};
    p2p::Protocol p2p_protocol_{};
    std::uint32_t p2p_magic_bits_{};
    std::uint16_t default_port_{};
    std::vector<std::string> dns_seeds_{};
    Amount default_fee_rate_{};  // satoshis per 1000 bytes
    display::Definition scales_{};
    std::size_t block_download_batch_{};
    ScriptMap scripts_{};
    StylePref styles_{};

#if OT_BLOCKCHAIN
    static auto Bip158() noexcept -> const FilterTypes&;
#endif  // OT_BLOCKCHAIN
    static auto Chains() noexcept -> const ChainData&;
#if OT_BLOCKCHAIN
    static auto Filters() noexcept -> const FilterData&;
    static auto Services() noexcept -> const ServiceBits&;
#endif  // OT_BLOCKCHAIN
};
}  // namespace opentxs::blockchain::params
