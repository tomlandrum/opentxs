// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

namespace opentxs::api::client::implementation
{
class OTX : virtual public api::client::internal::OTX, Lockable
{
public:
    BackgroundTask AcknowledgeBailment(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const Identifier& requestID,
        const std::string& instructions,
        const SetID setID) const override;
    BackgroundTask AcknowledgeNotice(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& recipientID,
        const Identifier& requestID,
        const bool ack,
        const SetID setID) const override;
    BackgroundTask AcknowledgeOutbailment(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& recipientID,
        const Identifier& requestID,
        const std::string& details,
        const SetID setID) const override;
    BackgroundTask AcknowledgeConnection(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& recipientID,
        const Identifier& requestID,
        const bool ack,
        const std::string& url,
        const std::string& login,
        const std::string& password,
        const std::string& key,
        const SetID setID) const override;
    bool AutoProcessInboxEnabled() const override
    {
        return auto_process_inbox_.get();
    };
    Depositability CanDeposit(
        const identifier::Nym& recipientNymID,
        const OTPayment& payment) const override;
    Depositability CanDeposit(
        const identifier::Nym& recipientNymID,
        const Identifier& accountID,
        const OTPayment& payment) const override;
    Messagability CanMessage(
        const identifier::Nym& senderNymID,
        const Identifier& recipientContactID,
        const bool startIntroductionServer) const override;
    bool CheckTransactionNumbers(
        const identifier::Nym& nym,
        const identifier::Server& serverID,
        const std::size_t quantity) const override;
    Finished ContextIdle(
        const identifier::Nym& nym,
        const identifier::Server& server) const override;
    std::size_t DepositCheques(const identifier::Nym& nymID) const override;
    std::size_t DepositCheques(
        const identifier::Nym& nymID,
        const std::set<OTIdentifier>& chequeIDs) const override;
    BackgroundTask DepositPayment(
        const identifier::Nym& recipientNymID,
        const std::shared_ptr<const OTPayment>& payment) const override;
    BackgroundTask DepositPayment(
        const identifier::Nym& recipientNymID,
        const Identifier& accountID,
        const std::shared_ptr<const OTPayment>& payment) const override;
    void DisableAutoaccept() const override;
#if OT_CASH
    BackgroundTask DownloadMint(
        const identifier::Nym& nym,
        const identifier::Server& server,
        const identifier::UnitDefinition& unit) const override;
#endif
    BackgroundTask DownloadNym(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID) const override;
    BackgroundTask DownloadNymbox(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID) const override;
    BackgroundTask DownloadServerContract(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Server& contractID) const override;
    BackgroundTask DownloadUnitDefinition(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::UnitDefinition& contractID) const override;
    BackgroundTask FindNym(const identifier::Nym& nymID) const override;
    BackgroundTask FindNym(
        const identifier::Nym& nymID,
        const identifier::Server& serverIDHint) const override;
    BackgroundTask FindServer(
        const identifier::Server& serverID) const override;
    BackgroundTask FindUnitDefinition(
        const identifier::UnitDefinition& unit) const override;
    BackgroundTask InitiateBailment(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const SetID setID) const override;
    BackgroundTask InitiateOutbailment(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const Amount amount,
        const std::string& message,
        const SetID setID) const override;
    BackgroundTask InitiateRequestConnection(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const proto::ConnectionInfoType& type,
        const SetID setID) const override;
    BackgroundTask InitiateStoreSecret(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const proto::SecretType& type,
        const std::string& primary,
        const std::string& secondary,
        const SetID setID) const override;
    const identifier::Server& IntroductionServer() const override;
    BackgroundTask IssueUnitDefinition(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::UnitDefinition& unitID,
        const std::string& label) const override;
    BackgroundTask MessageContact(
        const identifier::Nym& senderNymID,
        const Identifier& contactID,
        const std::string& message,
        const SetID setID) const override;
    std::pair<ThreadStatus, MessageID> MessageStatus(
        const TaskID taskID) const override;
    BackgroundTask NotifyBailment(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::Nym& targetNymID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const Identifier& requestID,
        const std::string& txid,
        const Amount amount,
        const SetID setID) const override;
    BackgroundTask PayContact(
        const identifier::Nym& senderNymID,
        const Identifier& contactID,
        std::shared_ptr<const OTPayment> payment) const override;
#if OT_CASH
    BackgroundTask PayContactCash(
        const identifier::Nym& senderNymID,
        const Identifier& contactID,
        const Identifier& workflowID) const override;
#endif  // OT_CASH
    BackgroundTask ProcessInbox(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& accountID) const override;
    BackgroundTask PublishServerContract(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& contractID) const override;
    void Refresh() const override;
    std::uint64_t RefreshCount() const override;
    BackgroundTask RegisterAccount(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::UnitDefinition& unitID,
        const std::string& label) const override;
    BackgroundTask RegisterNym(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const bool resync) const override;
    BackgroundTask RegisterNymPublic(
        const identifier::Nym& nymID,
        const identifier::Server& server,
        const bool setContactData,
        const bool forcePrimary,
        const bool resync) const override;
    OTServerID SetIntroductionServer(
        const ServerContract& contract) const override;
    BackgroundTask SendCheque(
        const identifier::Nym& localNymID,
        const Identifier& sourceAccountID,
        const Identifier& recipientContactID,
        const Amount value,
        const std::string& memo,
        const Time validFrom,
        const Time validTo) const override;
    BackgroundTask SendExternalTransfer(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& sourceAccountID,
        const Identifier& targetAccountID,
        const Amount value,
        const std::string& memo) const override;
    BackgroundTask SendTransfer(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const Identifier& sourceAccountID,
        const Identifier& targetAccountID,
        const Amount value,
        const std::string& memo) const override;
    void StartIntroductionServer(
        const identifier::Nym& localNymID) const override;
    ThreadStatus Status(const TaskID taskID) const override;
#if OT_CASH
    BackgroundTask WithdrawCash(
        const identifier::Nym& nymID,
        const identifier::Server& serverID,
        const Identifier& account,
        const Amount value) const override;
#endif  // OT_CASH

    ~OTX();

private:
    static const std::string DEFAULT_INTRODUCTION_SERVER;

    friend opentxs::Factory;

    using TaskStatusMap =
        std::map<TaskID, std::pair<ThreadStatus, std::promise<Result>>>;
    using ContextID = std::pair<OTNymID, OTServerID>;

    ContextLockCallback lock_callback_;
    const Flag& running_;
    const api::client::Manager& client_;
    OTClient& ot_client_;
    mutable std::mutex introduction_server_lock_{};
    mutable std::mutex nym_fetch_lock_{};
    mutable std::mutex task_status_lock_{};
    mutable std::atomic<std::uint64_t> refresh_counter_{0};
    mutable std::map<ContextID, StateMachine> operations_;
    mutable std::map<OTIdentifier, UniqueQueue<OTNymID>> server_nym_fetch_;
    UniqueQueue<CheckNymTask> missing_nyms_;
    UniqueQueue<CheckNymTask> outdated_nyms_;
    UniqueQueue<OTServerID> missing_servers_;
    UniqueQueue<OTUnitID> missing_unit_definitions_;
    mutable std::unique_ptr<OTServerID> introduction_server_id_;
    mutable TaskStatusMap task_status_;
    mutable std::map<TaskID, MessageID> task_message_id_;
    OTZMQListenCallback account_subscriber_callback_;
    OTZMQSubscribeSocket account_subscriber_;
    OTZMQListenCallback notification_listener_callback_;
    OTZMQPullSocket notification_listener_;
    OTZMQListenCallback find_nym_callback_;
    OTZMQPullSocket find_nym_listener_;
    OTZMQListenCallback find_server_callback_;
    OTZMQPullSocket find_server_listener_;
    OTZMQListenCallback find_unit_callback_;
    OTZMQPullSocket find_unit_listener_;
    OTZMQPublishSocket task_finished_;
    mutable OTFlag auto_process_inbox_;
    mutable std::atomic<TaskID> next_task_id_;
    mutable std::atomic<bool> shutdown_;
    mutable std::mutex shutdown_lock_;

    static BackgroundTask error_task();

    BackgroundTask add_task(const TaskID taskID, const ThreadStatus status)
        const;
    void associate_message_id(const Identifier& messageID, const TaskID taskID)
        const override;
    Depositability can_deposit(
        const OTPayment& payment,
        const identifier::Nym& recipient,
        const Identifier& accountIDHint,
        identifier::Server& depositServer,
        identifier::UnitDefinition& unitID,
        Identifier& depositAccount) const override;
    Messagability can_message(
        const identifier::Nym& senderNymID,
        const Identifier& recipientContactID,
        identifier::Nym& recipientNymID,
        identifier::Server& serverID) const;
    bool extract_payment_data(
        const OTPayment& payment,
        identifier::Nym& nymID,
        identifier::Server& serverID,
        identifier::UnitDefinition& unitID) const;
    void find_nym(const opentxs::network::zeromq::Message& message) const;
    void find_server(const opentxs::network::zeromq::Message& message) const;
    void find_unit(const opentxs::network::zeromq::Message& message) const;
    bool finish_task(const TaskID taskID, const bool success, Result&& result)
        const override;
    OTServerID get_introduction_server(const Lock& lock) const;
    UniqueQueue<OTNymID>& get_nym_fetch(
        const identifier::Server& serverID) const override;
    StateMachine& get_operations(const ContextID& id) const;
    StateMachine& get_task(const ContextID& id) const;
    OTServerID import_default_introduction_server(const Lock& lock) const;
    void load_introduction_server(const Lock& lock) const;
    TaskID next_task_id() const { return ++next_task_id_; }
    void process_account(
        const opentxs::network::zeromq::Message& message) const;
    void process_notification(
        const opentxs::network::zeromq::Message& message) const;
    bool publish_server_registration(
        const identifier::Nym& nymID,
        const identifier::Server& serverID,
        const bool forcePrimary) const;
    bool queue_cheque_deposit(
        const identifier::Nym& nymID,
        const Cheque& cheque) const;
    bool refresh_accounts() const;
    bool refresh_contacts() const;
    BackgroundTask schedule_download_nymbox(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID) const;
    BackgroundTask schedule_register_account(
        const identifier::Nym& localNymID,
        const identifier::Server& serverID,
        const identifier::UnitDefinition& unitID,
        const std::string& label) const;
    void set_contact(
        const identifier::Nym& nymID,
        const identifier::Server& serverID) const;
    OTServerID set_introduction_server(
        const Lock& lock,
        const ServerContract& contract) const;
    BackgroundTask start_task(const TaskID taskID, bool success) const override;
    ThreadStatus status(const Lock& lock, const TaskID taskID) const;
    void update_task(
        const TaskID taskID,
        const ThreadStatus status,
        Result&& result) const;
    void start_introduction_server(const identifier::Nym& nymID) const;
    void trigger_all() const;
    Depositability valid_account(
        const OTPayment& payment,
        const identifier::Nym& recipient,
        const identifier::Server& serverID,
        const identifier::UnitDefinition& unitID,
        const Identifier& accountIDHint,
        Identifier& depositAccount) const;
    bool valid_context(
        const identifier::Nym& nymID,
        const identifier::Server& serverID) const;
    Depositability valid_recipient(
        const OTPayment& payment,
        const identifier::Nym& specifiedNymID,
        const identifier::Nym& recipient) const;

    OTX(const Flag& running,
        const api::client::Manager& client,
        OTClient& otclient,
        const ContextLockCallback& lockCallback);
    OTX() = delete;
    OTX(const OTX&) = delete;
    OTX(OTX&&) = delete;
    OTX& operator=(const OTX&) = delete;
    OTX& operator=(OTX&&) = delete;
};
}  // namespace opentxs::api::client::implementation
