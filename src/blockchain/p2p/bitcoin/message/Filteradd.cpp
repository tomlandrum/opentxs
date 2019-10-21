// Copyright (c) 2010-2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "Internal.hpp"

#include "internal/blockchain/p2p/bitcoin/message/Message.hpp"

#include <set>

#include "Filteradd.hpp"

// #define OT_METHOD
// "opentxs::blockchain::p2p::bitcoin::message::implementation::Filteradd::"

namespace opentxs
{
blockchain::p2p::bitcoin::message::internal::Filteradd* Factory::
    BitcoinP2PFilteradd(
        const api::internal::Core& api,
        std::unique_ptr<blockchain::p2p::bitcoin::Header> pHeader,
        const blockchain::p2p::bitcoin::ProtocolVersion version,
        const void* payload,
        const std::size_t size)
{
    namespace bitcoin = blockchain::p2p::bitcoin;
    using ReturnType = bitcoin::message::implementation::Filteradd;

    if (false == bool(pHeader)) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": Invalid header")
            .Flush();

        return nullptr;
    }

    return new ReturnType(
        api, std::move(pHeader), Data::Factory(payload, size));
}

blockchain::p2p::bitcoin::message::internal::Filteradd* Factory::
    BitcoinP2PFilteradd(
        const api::internal::Core& api,
        const blockchain::Type network,
        const Data& element)
{
    namespace bitcoin = blockchain::p2p::bitcoin;
    using ReturnType = bitcoin::message::implementation::Filteradd;

    return new ReturnType(api, network, element);
}
}  // namespace opentxs

namespace opentxs::blockchain::p2p::bitcoin::message::implementation
{
Filteradd::Filteradd(
    const api::internal::Core& api,
    const blockchain::Type network,
    const Data& element) noexcept
    : Message(api, network, bitcoin::Command::filteradd)
    , element_(element)
{
    init_hash();
}

Filteradd::Filteradd(
    const api::internal::Core& api,
    std::unique_ptr<Header> header,
    const Data& element) noexcept
    : Message(api, std::move(header))
    , element_(element)
{
}

OTData Filteradd::payload() const noexcept
{
    try {
        const auto size = CompactSize(element_->size()).Encode();
        auto output = Data::Factory(size.data(), size.size());
        output += element_;

        return output;
    } catch (...) {
        return Data::Factory();
    }
}
}  // namespace opentxs::blockchain::p2p::bitcoin::message::implementation
