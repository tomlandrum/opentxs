// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "Internal.hpp"

#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/core/contract/peer/PeerObject.hpp"
#include "opentxs/core/Log.hpp"

#include "api/Factory.hpp"

#include "Factory.hpp"

//#define OT_METHOD "opentxs::api::client::implementation::Factory::"

namespace opentxs
{
api::Factory* Factory::FactoryAPIClient(const api::client::Manager& api)
{
    return new api::client::implementation::Factory(api);
}
}  // namespace opentxs

namespace opentxs::api::client::implementation
{
Factory::Factory(const api::client::Manager& client)
    : api::implementation::Factory(client)
    , client_(client)
{
}

std::unique_ptr<opentxs::PeerObject> Factory::PeerObject(
    const ConstNym& senderNym,
    const std::string& message) const
{
    return std::unique_ptr<opentxs::PeerObject>{
        opentxs::Factory::PeerObject(client_, senderNym, message)};
}

std::unique_ptr<opentxs::PeerObject> Factory::PeerObject(
    const ConstNym& senderNym,
    const std::string& payment,
    const bool isPayment) const
{
    return std::unique_ptr<opentxs::PeerObject>{
        opentxs::Factory::PeerObject(client_, senderNym, payment, isPayment)};
}

#if OT_CASH
std::unique_ptr<opentxs::PeerObject> Factory::PeerObject(
    const ConstNym& senderNym,
    const std::shared_ptr<blind::Purse> purse) const
{
    return std::unique_ptr<opentxs::PeerObject>{
        opentxs::Factory::PeerObject(client_, senderNym, purse)};
}
#endif

std::unique_ptr<opentxs::PeerObject> Factory::PeerObject(
    const std::shared_ptr<const PeerRequest> request,
    const std::shared_ptr<const PeerReply> reply,
    const std::uint32_t& version) const
{
    return std::unique_ptr<opentxs::PeerObject>{
        opentxs::Factory::PeerObject(client_, request, reply, version)};
}

std::unique_ptr<opentxs::PeerObject> Factory::PeerObject(
    const std::shared_ptr<const PeerRequest> request,
    const std::uint32_t& version) const
{
    return std::unique_ptr<opentxs::PeerObject>{
        opentxs::Factory::PeerObject(client_, request, version)};
}

std::unique_ptr<opentxs::PeerObject> Factory::PeerObject(
    const ConstNym& signerNym,
    const proto::PeerObject& serialized) const
{
    return std::unique_ptr<opentxs::PeerObject>{opentxs::Factory::PeerObject(
        client_.Contacts(), client_, signerNym, serialized)};
}

std::unique_ptr<opentxs::PeerObject> Factory::PeerObject(
    const ConstNym& recipientNym,
    const Armored& encrypted) const
{
    return std::unique_ptr<opentxs::PeerObject>{opentxs::Factory::PeerObject(
        client_.Contacts(), client_, recipientNym, encrypted)};
}
}  // namespace opentxs::api::client::implementation
