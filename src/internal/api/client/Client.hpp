// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

#include "opentxs/api/client/Activity.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/client/OTX.hpp"
#include "opentxs/consensus/ServerContext.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/UniqueQueue.hpp"

#include "internal/core/identifier/Identifier.hpp"
#include "internal/core/Core.hpp"

#include <future>
#include <string>
#include <tuple>

namespace opentxs
{
struct OT_DownloadNymboxType {
};
struct OT_GetTransactionNumbersType {
};
}  // namespace opentxs

namespace std
{
template <>
struct less<opentxs::OT_DownloadNymboxType> {
    bool operator()(
        const opentxs::OT_DownloadNymboxType&,
        const opentxs::OT_DownloadNymboxType&) const
    {
        return false;
    }
};
template <>
struct less<opentxs::OT_GetTransactionNumbersType> {
    bool operator()(
        const opentxs::OT_GetTransactionNumbersType&,
        const opentxs::OT_GetTransactionNumbersType&) const
    {
        return false;
    }
};
}  // namespace std

namespace opentxs::api::client
{
using CheckNymTask = OTNymID;
/** DepositPaymentTask: unit id, accountID, payment */
using DepositPaymentTask =
    std::tuple<OTUnitID, OTIdentifier, std::shared_ptr<const OTPayment>>;
using DownloadContractTask = OTServerID;
#if OT_CASH
using DownloadMintTask = std::pair<OTUnitID, int>;
#endif  // OT_CASH
using DownloadNymboxTask = OT_DownloadNymboxType;
using DownloadUnitDefinitionTask = OTUnitID;
using GetTransactionNumbersTask = OT_GetTransactionNumbersType;
/** IssueUnitDefinitionTask: unit definition id, account label */
using IssueUnitDefinitionTask = std::pair<OTUnitID, std::string>;
/** MessageTask: recipientID, message */
using MessageTask = std::tuple<OTNymID, std::string, std::shared_ptr<SetID>>;
#if OT_CASH
/** PayCashTask: recipientID, workflow ID */
using PayCashTask = std::pair<OTNymID, OTIdentifier>;
#endif  // OT_CASH
/** PaymentTask: recipientID, payment */
using PaymentTask = std::pair<OTNymID, std::shared_ptr<const OTPayment>>;
/** PeerReplyTask: targetNymID, peer reply, peer request */
using PeerReplyTask = std::tuple<
    OTNymID,
    std::shared_ptr<const PeerReply>,
    std::shared_ptr<const PeerRequest>>;
/** PeerRequestTask: targetNymID, peer request */
using PeerRequestTask = std::pair<OTNymID, std::shared_ptr<const PeerRequest>>;
using ProcessInboxTask = OTIdentifier;
using PublishServerContractTask = std::pair<OTServerID, bool>;
/** RegisterAccountTask: account label, unit definition id */
using RegisterAccountTask = std::pair<std::string, OTUnitID>;
using RegisterNymTask = bool;
/** SendChequeTask: sourceAccountID, targetNymID, value, memo, validFrom,
 * validTo
 */
using SendChequeTask =
    std::tuple<OTIdentifier, OTNymID, Amount, std::string, Time, Time>;
/** SendTransferTask: source account, destination account, amount, memo
 */
using SendTransferTask =
    std::tuple<OTIdentifier, OTIdentifier, Amount, std::string>;
#if OT_CASH
/** WithdrawCashTask: Account ID, amount*/
using WithdrawCashTask = std::pair<OTIdentifier, Amount>;
#endif  // OT_CASH
}  // namespace opentxs::api::client

namespace opentxs
{
template <>
struct make_blank<api::client::DepositPaymentTask> {
    static api::client::DepositPaymentTask value()
    {
        return {make_blank<OTUnitID>::value(),
                make_blank<OTIdentifier>::value(),
                nullptr};
    }
};
#if OT_CASH
template <>
struct make_blank<api::client::DownloadMintTask> {
    static api::client::DownloadMintTask value()
    {
        return {make_blank<OTUnitID>::value(), 0};
    }
};
#endif  // OT_CASH
template <>
struct make_blank<api::client::IssueUnitDefinitionTask> {
    static api::client::IssueUnitDefinitionTask value()
    {
        return {make_blank<OTUnitID>::value(), ""};
    }
};
template <>
struct make_blank<api::client::MessageTask> {
    static api::client::MessageTask value()
    {
        return {make_blank<OTNymID>::value(), "", nullptr};
    }
};
#if OT_CASH
template <>
struct make_blank<api::client::PayCashTask> {
    static api::client::PayCashTask value()
    {
        return {make_blank<OTNymID>::value(),
                make_blank<OTIdentifier>::value()};
    }
};
#endif  // OT_CASH
template <>
struct make_blank<api::client::PaymentTask> {
    static api::client::PaymentTask value()
    {
        return {make_blank<OTNymID>::value(), nullptr};
    }
};
template <>
struct make_blank<api::client::PeerReplyTask> {
    static api::client::PeerReplyTask value()
    {
        return {make_blank<OTNymID>::value(), nullptr, nullptr};
    }
};
template <>
struct make_blank<api::client::PeerRequestTask> {
    static api::client::PeerRequestTask value()
    {
        return {make_blank<OTNymID>::value(), nullptr};
    }
};
template <>
struct make_blank<api::client::PublishServerContractTask> {
    static api::client::PublishServerContractTask value()
    {
        return {make_blank<OTServerID>::value(), false};
    }
};
template <>
struct make_blank<api::client::RegisterAccountTask> {
    static api::client::RegisterAccountTask value()
    {
        return {"", make_blank<OTUnitID>::value()};
    }
};
template <>
struct make_blank<api::client::SendChequeTask> {
    static api::client::SendChequeTask value()
    {
        return {make_blank<OTIdentifier>::value(),
                make_blank<OTNymID>::value(),
                0,
                "",
                Clock::now(),
                Clock::now()};
    }
};
template <>
struct make_blank<api::client::SendTransferTask> {
    static api::client::SendTransferTask value()
    {
        return {make_blank<OTIdentifier>::value(),
                make_blank<OTIdentifier>::value(),
                0,
                ""};
    }
};
#if OT_CASH
template <>
struct make_blank<api::client::WithdrawCashTask> {
    static api::client::WithdrawCashTask value()
    {
        return {make_blank<OTIdentifier>::value(), 0};
    }
};
#endif  // OT_CASH
}  // namespace opentxs

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
    virtual const identifier::Server& ServerID() const = 0;

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
    virtual void Shutdown() = 0;
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

struct OTX : virtual public api::client::OTX {
    virtual void associate_message_id(
        const Identifier& messageID,
        const TaskID taskID) const = 0;
    virtual Depositability can_deposit(
        const OTPayment& payment,
        const identifier::Nym& recipient,
        const Identifier& accountIDHint,
        identifier::Server& depositServer,
        identifier::UnitDefinition& unitID,
        Identifier& depositAccount) const = 0;
    virtual bool finish_task(
        const TaskID taskID,
        const bool success,
        Result&& result) const = 0;
    virtual UniqueQueue<OTNymID>& get_nym_fetch(
        const identifier::Server& serverID) const = 0;
    virtual BackgroundTask start_task(const TaskID taskID, bool success)
        const = 0;
};
}  // namespace opentxs::api::client::internal
