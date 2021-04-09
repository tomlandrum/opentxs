// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"           // IWYU pragma: associated
#include "1_Internal.hpp"         // IWYU pragma: associated
#include "api/client/Wallet.hpp"  // IWYU pragma: associated

#include <functional>

#include "api/Wallet.hpp"
#include "internal/api/Api.hpp"
#include "internal/api/client/Client.hpp"
#include "internal/api/client/Factory.hpp"
#include "internal/otx/consensus/Consensus.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/network/ZMQ.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/socket/Publish.hpp"
#include "opentxs/otx/ConsensusType.hpp"
#include "opentxs/otx/consensus/Base.hpp"
#include "opentxs/otx/consensus/Server.hpp"
#include "opentxs/protobuf/ConsensusEnums.pb.h"
#include "opentxs/protobuf/Context.pb.h"
#include "opentxs/protobuf/ServerContext.pb.h"

//#define OT_METHOD "opentxs::api::client::implementation::Wallet::"

namespace opentxs::factory
{
auto Wallet(const api::client::internal::Manager& client) -> api::Wallet*
{
    return new api::client::implementation::Wallet(client);
}
}  // namespace opentxs::factory

namespace opentxs::api::client::implementation
{
Wallet::Wallet(const api::client::internal::Manager& client)
    : ot_super(client)
    , client_(client)
    , request_sent_(client.ZeroMQ().PublishSocket())
    , reply_received_(client.ZeroMQ().PublishSocket())
{
    auto bound = request_sent_->Start(api_.Endpoints().ServerRequestSent());
    bound &= reply_received_->Start(api_.Endpoints().ServerReplyReceived());

    OT_ASSERT(bound);
}

auto Wallet::Context(
    const identifier::Server& notaryID,
    const identifier::Nym& clientNymID) const
    -> std::shared_ptr<const otx::context::Base>
{
    auto serverID = Identifier::Factory(notaryID);

    return context(clientNymID, server_to_nym(serverID));
}

void Wallet::instantiate_server_context(
    const proto::Context& serialized,
    const Nym_p& localNym,
    const Nym_p& remoteNym,
    std::shared_ptr<otx::context::internal::Base>& output) const
{
    auto& zmq = client_.ZMQ();
    const auto& server = serialized.servercontext().serverid();
    auto& connection = zmq.Server(server);
    output.reset(factory::ServerContext(
        client_,
        request_sent_,
        reply_received_,
        serialized,
        localNym,
        remoteNym,
        connection));
}

auto Wallet::mutable_Context(
    const identifier::Server& notaryID,
    const identifier::Nym& clientNymID,
    const PasswordPrompt& reason) const -> Editor<otx::context::Base>
{
    auto serverID = Identifier::Factory(notaryID);
    auto base = context(clientNymID, server_to_nym(serverID));
    std::function<void(otx::context::Base*)> callback =
        [&](otx::context::Base* in) -> void {
        this->save(reason, dynamic_cast<otx::context::internal::Base*>(in));
    };

    OT_ASSERT(base);

    return Editor<otx::context::Base>(base.get(), callback);
}

auto Wallet::mutable_ServerContext(
    const identifier::Nym& localNymID,
    const Identifier& remoteID,
    const PasswordPrompt& reason) const -> Editor<otx::context::Server>
{
    Lock lock(context_map_lock_);

    auto serverID = Identifier::Factory(remoteID.str());
    const auto remoteNymID = server_to_nym(serverID);

    auto base = context(localNymID, remoteNymID);

    std::function<void(otx::context::Base*)> callback =
        [&](otx::context::Base* in) -> void {
        this->save(reason, dynamic_cast<otx::context::internal::Base*>(in));
    };

    if (base) {
        OT_ASSERT(otx::ConsensusType::Server == base->Type());
    } else {
        // Obtain nyms.
        const auto localNym = Nym(localNymID);

        OT_ASSERT_MSG(localNym, "Local nym does not exist in the wallet.");

        const auto remoteNym = Nym(remoteNymID);

        OT_ASSERT_MSG(remoteNym, "Remote nym does not exist in the wallet.");

        // Create a new Context
        const ContextID contextID = {localNymID.str(), remoteNymID->str()};
        auto& entry = context_map_[contextID];
        auto& zmq = client_.ZMQ();
        auto& connection = zmq.Server(serverID->str());
        entry.reset(factory::ServerContext(
            client_,
            request_sent_,
            reply_received_,
            localNym,
            remoteNym,
            identifier::Server::Factory(serverID->str()),  // TODO conversion
            connection));
        base = entry;
    }

    OT_ASSERT(base);

    auto child = dynamic_cast<otx::context::Server*>(base.get());

    OT_ASSERT(nullptr != child);

    return Editor<otx::context::Server>(child, callback);
}

void Wallet::nym_to_contact(const identity::Nym& nym, const std::string& name)
    const noexcept
{
    auto code = api_.Factory().PaymentCode(nym.PaymentCode());
    client_.Contacts().NewContact(name, nym.ID(), code);
}

auto Wallet::ServerContext(
    const identifier::Nym& localNymID,
    const Identifier& remoteID) const
    -> std::shared_ptr<const otx::context::Server>
{
    auto serverID = Identifier::Factory(remoteID);
    auto remoteNymID = server_to_nym(serverID);
    auto base = context(localNymID, remoteNymID);

    auto output = std::dynamic_pointer_cast<const otx::context::Server>(base);

    return output;
}

auto Wallet::signer_nym(const identifier::Nym& id) const -> Nym_p
{
    return Nym(id);
}
}  // namespace opentxs::api::client::implementation
