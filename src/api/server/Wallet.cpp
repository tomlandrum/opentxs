// Copyright (c) 2010-2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "Internal.hpp"

#include "opentxs/api/server/Manager.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/consensus/ClientContext.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/network/zeromq/socket/Push.hpp"

#include "api/Wallet.hpp"
#include "internal/api/server/Server.hpp"

#include "Wallet.hpp"

#define OT_METHOD "opentxs::api::server::implementation::Wallet::"

namespace opentxs
{
api::Wallet* Factory::Wallet(const api::server::internal::Manager& server)
{
    return new api::server::implementation::Wallet(server);
}
}  // namespace opentxs

namespace opentxs::api::server::implementation
{
Wallet::Wallet(const api::server::internal::Manager& server)
    : ot_super(server)
    , server_(server)
{
}

std::shared_ptr<const opentxs::ClientContext> Wallet::ClientContext(
    const identifier::Nym& remoteNymID,
    const PasswordPrompt& reason) const
{
    const auto& serverNymID = server_.NymID();
    auto base = context(serverNymID, remoteNymID, reason);
    auto output = std::dynamic_pointer_cast<const opentxs::ClientContext>(base);

    return output;
}

std::shared_ptr<const opentxs::Context> Wallet::Context(
    [[maybe_unused]] const identifier::Server& notaryID,
    const identifier::Nym& clientNymID,
    const PasswordPrompt& reason) const
{
    return context(server_.NymID(), clientNymID, reason);
}

void Wallet::instantiate_client_context(
    const proto::Context& serialized,
    const Nym_p& localNym,
    const Nym_p& remoteNym,
    std::shared_ptr<opentxs::internal::Context>& output) const
{
    output.reset(opentxs::Factory::ClientContext(
        api_, serialized, localNym, remoteNym, server_.ID()));
}

bool Wallet::load_legacy_account(
    const PasswordPrompt& reason,
    const Identifier& accountID,
    const eLock& lock,
    Wallet::AccountLock& row) const
{
    // WTF clang? This is perfectly valid c++17. Fix your shit.
    // auto& [rowMutex, pAccount] = row;
    const auto& rowMutex = std::get<0>(row);
    auto& pAccount = std::get<1>(row);

    OT_ASSERT(CheckLock(lock, rowMutex))

    pAccount.reset(
        Account::LoadExistingAccount(api_, accountID, server_.ID(), reason));

    if (false == bool(pAccount)) { return false; }

    const auto signerNym = Nym(server_.NymID(), reason);

    if (false == bool(signerNym)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to load signer nym.")
            .Flush();

        return false;
    }

    if (false == pAccount->VerifySignature(*signerNym, reason)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid signature.").Flush();

        return false;
    }

    LogOutput(OT_METHOD)(__FUNCTION__)(": Legacy account ")(accountID.str())(
        " exists.")
        .Flush();

    auto serialized = String::Factory();
    auto saved = pAccount->SaveContractRaw(serialized);

    OT_ASSERT(saved)

    const auto& ownerID = pAccount->GetNymID();

    OT_ASSERT(false == ownerID.empty())

    const auto& unitID = pAccount->GetInstrumentDefinitionID();

    OT_ASSERT(false == unitID.empty())

    const auto contract = UnitDefinition(unitID, reason);
    const auto& serverID = pAccount->GetPurportedNotaryID();

    OT_ASSERT(server_.ID() == serverID)

    saved = api_.Storage().Store(
        accountID.str(),
        serialized->Get(),
        "",
        ownerID,
        server_.NymID(),
        contract->Nym()->ID(),
        serverID,
        unitID,
        extract_unit(reason, unitID));

    OT_ASSERT(saved)

    return true;
}

Editor<opentxs::ClientContext> Wallet::mutable_ClientContext(
    const identifier::Nym& remoteNymID,
    const PasswordPrompt& reason) const
{
    const auto& serverID = server_.ID();
    const auto& serverNymID = server_.NymID();
    Lock lock(context_map_lock_);
    auto base = context(serverNymID, remoteNymID, reason);
    std::function<void(opentxs::Context*)> callback =
        [&](opentxs::Context* in) -> void {
        this->save(reason, dynamic_cast<opentxs::internal::Context*>(in));
    };

    if (base) {
        OT_ASSERT(proto::CONSENSUSTYPE_CLIENT == base->Type());
    } else {
        // Obtain nyms.
        const auto local = Nym(serverNymID, reason);

        OT_ASSERT_MSG(local, "Local nym does not exist in the wallet.");

        const auto remote = Nym(remoteNymID, reason);

        OT_ASSERT_MSG(remote, "Remote nym does not exist in the wallet.");

        // Create a new Context
        const ContextID contextID = {serverNymID.str(), remoteNymID.str()};
        auto& entry = context_map_[contextID];
        entry.reset(
            opentxs::Factory::ClientContext(api_, local, remote, serverID));
        base = entry;
    }

    OT_ASSERT(base);

    auto child = dynamic_cast<opentxs::ClientContext*>(base.get());

    OT_ASSERT(nullptr != child);

    return Editor<opentxs::ClientContext>(child, callback);
}

Editor<opentxs::Context> Wallet::mutable_Context(
    const identifier::Server& notaryID,
    const identifier::Nym& clientNymID,
    const PasswordPrompt& reason) const
{
    auto base = context(server_.NymID(), clientNymID, reason);
    std::function<void(opentxs::Context*)> callback =
        [&](opentxs::Context* in) -> void {
        this->save(reason, dynamic_cast<opentxs::internal::Context*>(in));
    };

    OT_ASSERT(base);

    return Editor<opentxs::Context>(base.get(), callback);
}

Nym_p Wallet::signer_nym(const identifier::Nym&, const PasswordPrompt& reason)
    const
{
    return Nym(server_.NymID(), reason);
}
}  // namespace opentxs::api::server::implementation
