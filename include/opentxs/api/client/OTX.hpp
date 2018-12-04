// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_API_CLIENT_OTX_HPP
#define OPENTXS_API_CLIENT_OTX_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"

#include <chrono>
#include <cstdint>
#include <future>
#include <tuple>
#include <memory>
#include <set>

#define OT_CHEQUE_DAYS 30
#define OT_CHEQUE_HOURS 24 * OT_CHEQUE_DAYS
#define DEFAULT_PROCESS_INBOX_ITEMS 5

namespace opentxs
{
namespace api
{
namespace client
{
class OTX
{
public:
    using Clock = std::chrono::system_clock;
    using Time = Clock::time_point;
    using TaskID = int;
    using MessageID = OTIdentifier;
    using Result = std::pair<proto::LastReplyStatus, std::shared_ptr<Message>>;
    using Future = std::future<Result>;
    using BackgroundTask = std::pair<TaskID, Future>;
    using Finished = std::future<void>;

    EXPORT virtual BackgroundTask AcknowledgeBailment(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const Identifier& requestID,
        const std::string& instructions,
        const SetID setID = {}) const = 0;
    EXPORT virtual BackgroundTask AcknowledgeNotice(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& recipientID,
        const Identifier& requestID,
        const bool ack,
        const SetID setID = {}) const = 0;
    EXPORT virtual BackgroundTask AcknowledgeOutbailment(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& recipientID,
        const Identifier& requestID,
        const std::string& details,
        const SetID setID = {}) const = 0;
    EXPORT virtual BackgroundTask AcknowledgeConnection(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& recipientID,
        const Identifier& requestID,
        const bool ack,
        const std::string& url,
        const std::string& login,
        const std::string& password,
        const std::string& key,
        const SetID setID = {}) const = 0;
    EXPORT virtual bool AutoProcessInboxEnabled() const = 0;
    EXPORT virtual Depositability CanDeposit(
        const Identifier& recipientNymID,
        const OTPayment& payment) const = 0;
    EXPORT virtual Depositability CanDeposit(
        const Identifier& recipientNymID,
        const Identifier& accountID,
        const OTPayment& payment) const = 0;
    EXPORT virtual Messagability CanMessage(
        const Identifier& senderNymID,
        const Identifier& recipientContactID,
        const bool startIntroductionServer = true) const = 0;
    EXPORT virtual bool CheckTransactionNumbers(
        const identifier::Nym& nym,
        const Identifier& serverID,
        const std::size_t quantity) const = 0;
    EXPORT virtual Finished ContextIdle(
        const identifier::Nym& nym,
        const identifier::Server& server) const = 0;
    /** Deposit all available cheques for specified nym
     *
     *  \returns the number of cheques queued for deposit
     */
    EXPORT virtual std::size_t DepositCheques(
        const Identifier& nymID) const = 0;
    /** Deposit the specified list of cheques for specified nym
     *
     *  If the list of chequeIDs is empty, then all cheques will be deposited
     *
     *  \returns the number of cheques queued for deposit
     */
    EXPORT virtual std::size_t DepositCheques(
        const Identifier& nymID,
        const std::set<OTIdentifier>& chequeIDs) const = 0;
    EXPORT virtual BackgroundTask DepositPayment(
        const Identifier& recipientNymID,
        const std::shared_ptr<const OTPayment>& payment) const = 0;
    EXPORT virtual BackgroundTask DepositPayment(
        const Identifier& recipientNymID,
        const Identifier& accountID,
        const std::shared_ptr<const OTPayment>& payment) const = 0;
    /** Used by unit tests */
    EXPORT virtual void DisableAutoaccept() const = 0;
    EXPORT virtual BackgroundTask DownloadContract(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& contractID) const = 0;
#if OT_CASH
    EXPORT virtual BackgroundTask DownloadMint(
        const identifier::Nym& nym,
        const identifier::Server& server,
        const identifier::UnitDefinition& unit) const = 0;
#endif
    EXPORT virtual BackgroundTask DownloadNym(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& targetNymID) const = 0;
    EXPORT virtual BackgroundTask DownloadNymbox(
        const Identifier& localNymID,
        const Identifier& serverID) const = 0;
    EXPORT virtual BackgroundTask FindNym(const Identifier& nymID) const = 0;
    EXPORT virtual BackgroundTask FindNym(
        const Identifier& nymID,
        const Identifier& serverIDHint) const = 0;
    EXPORT virtual BackgroundTask FindServer(
        const Identifier& serverID) const = 0;
    EXPORT virtual BackgroundTask InitiateBailment(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const SetID setID = {}) const = 0;
    EXPORT virtual BackgroundTask InitiateOutbailment(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const Amount amount,
        const std::string& message,
        const SetID setID = {}) const = 0;
    EXPORT virtual BackgroundTask InitiateRequestConnection(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const proto::ConnectionInfoType& type,
        const SetID setID = {}) const = 0;
    EXPORT virtual BackgroundTask InitiateStoreSecret(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const proto::SecretType& type,
        const std::string& primary,
        const std::string& secondary,
        const SetID setID = {}) const = 0;
    EXPORT virtual const Identifier& IntroductionServer() const = 0;
    EXPORT virtual BackgroundTask IssueUnitDefinition(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& unitIDD,
        const std::string& label = "") const = 0;
    EXPORT virtual BackgroundTask MessageContact(
        const Identifier& senderNymID,
        const Identifier& contactID,
        const std::string& message,
        const SetID setID = {}) const = 0;
    EXPORT virtual std::pair<ThreadStatus, MessageID> MessageStatus(
        const TaskID taskID) const = 0;
    EXPORT virtual BackgroundTask NotifyBailment(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const Identifier& requestID,
        const std::string& txid,
        const Amount amount,
        const SetID setID = {}) const = 0;
    EXPORT virtual BackgroundTask PayContact(
        const Identifier& senderNymID,
        const Identifier& contactID,
        std::shared_ptr<const OTPayment> payment) const = 0;
#if OT_CASH
    EXPORT virtual BackgroundTask PayContactCash(
        const Identifier& senderNymID,
        const Identifier& contactID,
        const Identifier& workflowID) const = 0;
#endif  // OT_CASH
    EXPORT virtual BackgroundTask ProcessInbox(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& accountID) const = 0;
    EXPORT virtual BackgroundTask PublishServerContract(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& contractID) const = 0;
    EXPORT virtual void Refresh() const = 0;
    EXPORT virtual std::uint64_t RefreshCount() const = 0;
    EXPORT virtual BackgroundTask RegisterAccount(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& unitID,
        const std::string& label = "") const = 0;
    EXPORT virtual BackgroundTask RegisterNym(
        const Identifier& localNymID,
        const Identifier& serverID,
        const bool resync = false) const = 0;
    EXPORT virtual BackgroundTask RegisterNymPublic(
        const Identifier& nymID,
        const Identifier& server,
        const bool setContactData,
        const bool forcePrimary = false,
        const bool resync = false) const = 0;
    EXPORT virtual OTIdentifier SetIntroductionServer(
        const ServerContract& contract) const = 0;
    EXPORT virtual BackgroundTask SendCheque(
        const Identifier& localNymID,
        const Identifier& sourceAccountID,
        const Identifier& recipientContactID,
        const Amount value,
        const std::string& memo,
        const Time validFrom = Clock::now(),
        const Time validTo =
            (Clock::now() + std::chrono::hours(OT_CHEQUE_HOURS))) const = 0;
    EXPORT virtual BackgroundTask SendExternalTransfer(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& sourceAccountID,
        const Identifier& targetAccountID,
        const Amount value,
        const std::string& memo) const = 0;
    EXPORT virtual BackgroundTask SendTransfer(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& sourceAccountID,
        const Identifier& targetAccountID,
        const Amount value,
        const std::string& memo) const = 0;
    EXPORT virtual void StartIntroductionServer(
        const Identifier& localNymID) const = 0;
    EXPORT virtual ThreadStatus Status(const TaskID taskID) const = 0;
#if OT_CASH
    EXPORT virtual BackgroundTask WithdrawCash(
        const identifier::Nym& nymID,
        const identifier::Server& serverID,
        const Identifier& account,
        const Amount value) const = 0;
#endif

    EXPORT virtual ~OTX() = default;

protected:
    OTX() = default;

private:
    OTX(const OTX&) = delete;
    OTX(OTX&&) = delete;
    OTX& operator=(const OTX&) = delete;
    OTX& operator=(OTX&&) = delete;
};
}  // namespace client
}  // namespace api
}  // namespace opentxs
#endif
