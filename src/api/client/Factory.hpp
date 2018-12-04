// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace opentxs::api::client::implementation
{
class Factory final : public opentxs::api::implementation::Factory
{
public:
    std::unique_ptr<opentxs::PeerObject> PeerObject(
        const ConstNym& senderNym,
        const std::string& message) const override;
    std::unique_ptr<opentxs::PeerObject> PeerObject(
        const ConstNym& senderNym,
        const std::string& payment,
        const bool isPayment) const override;
#if OT_CASH
    std::unique_ptr<opentxs::PeerObject> PeerObject(
        const ConstNym& senderNym,
        const std::shared_ptr<blind::Purse> purse) const override;
#endif
    std::unique_ptr<opentxs::PeerObject> PeerObject(
        const std::shared_ptr<const PeerRequest> request,
        const std::shared_ptr<const PeerReply> reply,
        const std::uint32_t& version) const override;
    std::unique_ptr<opentxs::PeerObject> PeerObject(
        const std::shared_ptr<const PeerRequest> request,
        const std::uint32_t& version) const override;
    std::unique_ptr<opentxs::PeerObject> PeerObject(
        const ConstNym& signerNym,
        const proto::PeerObject& serialized) const override;
    std::unique_ptr<opentxs::PeerObject> PeerObject(
        const ConstNym& recipientNym,
        const Armored& encrypted) const override;

    ~Factory() override = default;

private:
    friend opentxs::Factory;

    const api::client::Manager& client_;

    Factory(const api::client::Manager& client);
    Factory() = delete;
    Factory(const Factory&) = delete;
    Factory(Factory&&) = delete;
    Factory& operator=(const Factory&) = delete;
    Factory& operator=(Factory&&) = delete;
};
}  // namespace opentxs::api::client::implementation
