// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace opentxs
{
namespace api
{
namespace client
{
class Contacts;
}  // namespace client
}  // namespace api
}  // namespace opentxs

namespace opentxs::factory
{
auto PeerObject(
    const api::internal::Core& api,
    const Nym_p& senderNym,
    const std::string& message) -> opentxs::PeerObject*;
auto PeerObject(
    const api::internal::Core& api,
    const Nym_p& senderNym,
    const std::string& payment,
    const bool isPayment) -> opentxs::PeerObject*;
#if OT_CASH
auto PeerObject(
    const api::internal::Core& api,
    const Nym_p& senderNym,
    const std::shared_ptr<blind::Purse> purse) -> opentxs::PeerObject*;
#endif
auto PeerObject(
    const api::internal::Core& api,
    const OTPeerRequest request,
    const OTPeerReply reply,
    const VersionNumber version) -> opentxs::PeerObject*;
auto PeerObject(
    const api::internal::Core& api,
    const OTPeerRequest request,
    const VersionNumber version) -> opentxs::PeerObject*;
auto PeerObject(
    const api::client::Contacts& contacts,
    const api::internal::Core& api,
    const Nym_p& signerNym,
    const proto::PeerObject& serialized) -> opentxs::PeerObject*;
auto PeerObject(
    const api::client::Contacts& contacts,
    const api::internal::Core& api,
    const Nym_p& recipientNym,
    const opentxs::Armored& encrypted,
    const opentxs::PasswordPrompt& reason) -> opentxs::PeerObject*;
auto PeerReply(const api::Core& api) noexcept
    -> std::shared_ptr<contract::peer::Reply>;

auto PeerRequest(const api::Core& api) noexcept
    -> std::shared_ptr<contract::peer::Request>;
}  // namespace opentxs::factory
