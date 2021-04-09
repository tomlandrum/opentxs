// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"           // IWYU pragma: associated
#include "1_Internal.hpp"         // IWYU pragma: associated
#include "api/server/Wallet.hpp"  // IWYU pragma: associated

#include <functional>
#include <utility>

#include "2_Factory.hpp"
#include "api/Wallet.hpp"
#include "internal/api/Api.hpp"
#include "internal/api/server/Server.hpp"
#include "internal/otx/consensus/Consensus.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Lockable.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/otx/ConsensusType.hpp"
#include "opentxs/otx/consensus/Base.hpp"
#include "opentxs/otx/consensus/Client.hpp"
#include "opentxs/protobuf/ConsensusEnums.pb.h"

#define OT_METHOD "opentxs::api::server::implementation::Wallet::"

namespace opentxs
{
auto Factory::Wallet(const api::server::internal::Manager& server)
    -> api::Wallet*
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

auto Wallet::ClientContext(const identifier::Nym& remoteNymID) const
    -> std::shared_ptr<const otx::context::Client>
{
    const auto& serverNymID = server_.NymID();
    auto base = context(serverNymID, remoteNymID);
    auto output = std::dynamic_pointer_cast<const otx::context::Client>(base);

    return output;
}

auto Wallet::Context(
    [[maybe_unused]] const identifier::Server& notaryID,
    const identifier::Nym& clientNymID) const
    -> std::shared_ptr<const otx::context::Base>
{
    return context(server_.NymID(), clientNymID);
}

void Wallet::instantiate_client_context(
    const proto::Context& serialized,
    const Nym_p& localNym,
    const Nym_p& remoteNym,
    std::shared_ptr<otx::context::internal::Base>& output) const
{
    output.reset(factory::ClientContext(
        api_, serialized, localNym, remoteNym, server_.ID()));
}

auto Wallet::load_legacy_account(
    const Identifier& accountID,
    const eLock& lock,
    Wallet::AccountLock& row) const -> bool
{
    // WTF clang? This is perfectly valid c++17. Fix your shit.
    // auto& [rowMutex, pAccount] = row;
    const auto& rowMutex = std::get<0>(row);
    auto& pAccount = std::get<1>(row);

    OT_ASSERT(CheckLock(lock, rowMutex))

    pAccount.reset(Account::LoadExistingAccount(api_, accountID, server_.ID()));

    if (false == bool(pAccount)) { return false; }

    const auto signerNym = Nym(server_.NymID());

    if (false == bool(signerNym)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to load signer nym.")
            .Flush();

        return false;
    }

    if (false == pAccount->VerifySignature(*signerNym)) {
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

    const auto contract = UnitDefinition(unitID);
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
        extract_unit(unitID));

    OT_ASSERT(saved)

    return true;
}

auto Wallet::mutable_ClientContext(
    const identifier::Nym& remoteNymID,
    const PasswordPrompt& reason) const -> Editor<otx::context::Client>
{
    const auto& serverID = server_.ID();
    const auto& serverNymID = server_.NymID();
    Lock lock(context_map_lock_);
    auto base = context(serverNymID, remoteNymID);
    std::function<void(otx::context::Base*)> callback =
        [&](otx::context::Base* in) -> void {
        this->save(reason, dynamic_cast<otx::context::internal::Base*>(in));
    };

    if (base) {
        OT_ASSERT(otx::ConsensusType::Client == base->Type());
    } else {
        // Obtain nyms.
        const auto local = Nym(serverNymID);

        OT_ASSERT_MSG(local, "Local nym does not exist in the wallet.");

        const auto remote = Nym(remoteNymID);

        OT_ASSERT_MSG(remote, "Remote nym does not exist in the wallet.");

        // Create a new Context
        const ContextID contextID = {serverNymID.str(), remoteNymID.str()};
        auto& entry = context_map_[contextID];
        entry.reset(factory::ClientContext(api_, local, remote, serverID));
        base = entry;
    }

    OT_ASSERT(base);

    auto child = dynamic_cast<otx::context::Client*>(base.get());

    OT_ASSERT(nullptr != child);

    return Editor<otx::context::Client>(child, callback);
}

auto Wallet::mutable_Context(
    const identifier::Server& notaryID,
    const identifier::Nym& clientNymID,
    const PasswordPrompt& reason) const -> Editor<otx::context::Base>
{
    auto base = context(server_.NymID(), clientNymID);
    std::function<void(otx::context::Base*)> callback =
        [&](otx::context::Base* in) -> void {
        this->save(reason, dynamic_cast<otx::context::internal::Base*>(in));
    };

    OT_ASSERT(base);

    return Editor<otx::context::Base>(base.get(), callback);
}

auto Wallet::signer_nym(const identifier::Nym&) const -> Nym_p
{
    return Nym(server_.NymID());
}
}  // namespace opentxs::api::server::implementation
