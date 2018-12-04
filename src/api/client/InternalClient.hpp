// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

#include "opentxs/api/client/Activity.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/consensus/ServerContext.hpp"

#include <future>

namespace opentxs::api::client::internal
{
struct Activity : virtual public api::client::Activity {
    virtual void MigrateLegacyThreads() const = 0;

    virtual ~Activity() = default;
};
struct Contacts : virtual public api::client::Contacts {
    virtual void start() = 0;

    virtual ~Contacts() = default;
};
struct Manager : virtual public api::client::Manager {
    virtual void StartActivity() = 0;
    virtual void StartContacts() = 0;
    virtual opentxs::OTWallet* StartWallet() = 0;

    virtual ~Manager() = default;
};
struct Operation {
    using Result = ServerContext::DeliveryResult;
    using Future = std::future<Result>;

    enum class Type {
        Invalid = 0,
        AddClaim,
        CheckNym,
        ConveyPayment,
        DepositCash,
        DepositCheque,
        DownloadContract,
        DownloadMint,
        GetTransactionNumbers,
        IssueUnitDefinition,
        PublishNym,
        PublishServer,
        PublishUnit,
        RefreshAccount,
        RegisterAccount,
        RegisterNym,
        RequestAdmin,
        SendCash,
        SendMessage,
        SendPeerReply,
        SendPeerRequest,
        SendTransfer,
        WithdrawCash,
    };

    virtual const identifier::Nym& NymID() const = 0;
    virtual const Identifier& ServerID() const = 0;

    virtual bool AddClaim(
        const proto::ContactSectionName section,
        const proto::ContactItemType type,
        const String& value,
        const bool primary) = 0;
    virtual bool ConveyPayment(
        const identifier::Nym& recipient,
        const std::shared_ptr<const OTPayment> payment) = 0;
#if OT_CASH
    virtual bool DepositCash(
        const Identifier& depositAccountID,
        const std::shared_ptr<blind::Purse> purse) = 0;
#endif
    virtual bool DepositCheque(
        const Identifier& depositAccountID,
        const std::shared_ptr<Cheque> cheque) = 0;
    virtual bool DownloadContract(
        const Identifier& ID,
        const ContractType type = ContractType::ERROR) = 0;
    virtual Future GetFuture() = 0;
    virtual bool IssueUnitDefinition(
        const std::shared_ptr<const proto::UnitDefinition> unitDefinition,
        const ServerContext::ExtraArgs& args = {}) = 0;
    virtual void join() = 0;
    virtual bool PublishContract(const identifier::Nym& id) = 0;
    virtual bool PublishContract(const identifier::Server& id) = 0;
    virtual bool PublishContract(const identifier::UnitDefinition& id) = 0;
    virtual bool RequestAdmin(const String& password) = 0;
#if OT_CASH
    virtual bool SendCash(
        const identifier::Nym& recipient,
        const Identifier& workflowID) = 0;
#endif
    virtual bool SendMessage(
        const identifier::Nym& recipient,
        const String& message,
        const SetID setID = {}) = 0;
    virtual bool SendPeerReply(
        const identifier::Nym& targetNymID,
        const std::shared_ptr<const PeerReply> peerreply,
        const std::shared_ptr<const PeerRequest> peerrequest) = 0;
    virtual bool SendPeerRequest(
        const identifier::Nym& targetNymID,
        const std::shared_ptr<const PeerRequest> peerrequest) = 0;
    virtual bool SendTransfer(
        const Identifier& sourceAccountID,
        const Identifier& destinationAccountID,
        const Amount amount,
        const String& memo) = 0;
    virtual void SetPush(const bool enabled) = 0;
    virtual bool Start(
        const Type type,
        const ServerContext::ExtraArgs& args = {}) = 0;
    virtual bool Start(
        const Type type,
        const identifier::UnitDefinition& targetUnitID,
        const ServerContext::ExtraArgs& args = {}) = 0;
    virtual bool Start(
        const Type type,
        const identifier::Nym& targetNymID,
        const ServerContext::ExtraArgs& args = {}) = 0;
    virtual bool UpdateAccount(const Identifier& accountID) = 0;
#if OT_CASH
    virtual bool WithdrawCash(
        const Identifier& accountID,
        const Amount amount) = 0;
#endif

    virtual ~Operation() = default;
};
}  // namespace opentxs::api::client::internal
