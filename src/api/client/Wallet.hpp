// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace opentxs::api::client::implementation
{
class Wallet final : public api::implementation::Wallet
{
public:
    std::shared_ptr<const opentxs::Context> Context(
        const Identifier& notaryID,
        const Identifier& clientNymID) const override;
    Editor<opentxs::Context> mutable_Context(
        const Identifier& notaryID,
        const Identifier& clientNymID) const override;
    Editor<opentxs::ServerContext> mutable_ServerContext(
        const Identifier& localNymID,
        const Identifier& remoteID) const override;
    std::shared_ptr<const opentxs::ServerContext> ServerContext(
        const Identifier& localNymID,
        const Identifier& remoteID) const override;

    ~Wallet() = default;

private:
    friend opentxs::Factory;

    using ot_super = api::implementation::Wallet;

    const api::client::Manager& client_;
    OTZMQPublishSocket request_sent_;
    OTZMQPublishSocket reply_received_;

    void instantiate_server_context(
        const proto::Context& serialized,
        const std::shared_ptr<const opentxs::Nym>& localNym,
        const std::shared_ptr<const opentxs::Nym>& remoteNym,
        std::shared_ptr<opentxs::internal::Context>& output) const override;
    std::shared_ptr<const opentxs::Nym> signer_nym(
        const Identifier& id) const override;

    Wallet(const api::client::Manager& client);
    Wallet() = delete;
    Wallet(const Wallet&) = delete;
    Wallet(Wallet&&) = delete;
    Wallet& operator=(const Wallet&) = delete;
    Wallet& operator=(Wallet&&) = delete;
};
}  // namespace opentxs::api::client::implementation
