// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <atomic>
#include <cstdint>
#include <future>
#include <iosfwd>
#include <map>
#include <memory>
#include <set>

#include "core/StateMachine.hpp"
#include "internal/otx/client/Client.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/Editor.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Ledger.hpp"
#include "opentxs/core/Message.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/contract/peer/PeerReply.hpp"
#include "opentxs/core/contract/peer/PeerRequest.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/otx/consensus/ManagedNumber.hpp"
#include "opentxs/otx/consensus/Server.hpp"

namespace opentxs
{
namespace api
{
namespace client
{
namespace internal
{
struct Manager;
}  // namespace internal
}  // namespace client
}  // namespace api

namespace blind
{
class Purse;
}  // namespace blind

namespace otx
{
namespace context
{
class Base;
}  // namespace context
}  // namespace otx

namespace proto
{
class UnitDefinition;
}  // namespace proto

class Armored;
class Cheque;
class Context;
class Factory;
class OTPayment;
class OTTransaction;
class PeerObject;
}  // namespace opentxs

namespace opentxs::otx::client::implementation
{
class Operation final : virtual public otx::client::internal::Operation,
                        public opentxs::internal::StateMachine
{
public:
    auto NymID() const -> const identifier::Nym& override { return nym_id_; }
    auto ServerID() const -> const identifier::Server& override
    {
        return server_id_;
    }

    auto AddClaim(
        const contact::ContactSectionName section,
        const contact::ContactItemType type,
        const String& value,
        const bool primary) -> bool override;
    auto ConveyPayment(
        const identifier::Nym& recipient,
        const std::shared_ptr<const OTPayment> payment) -> bool override;
#if OT_CASH
    auto DepositCash(
        const Identifier& depositAccountID,
        const std::shared_ptr<blind::Purse> purse) -> bool override;
#endif
    auto DepositCheque(
        const Identifier& depositAccountID,
        const std::shared_ptr<Cheque> cheque) -> bool override;
    auto DownloadContract(const Identifier& ID, const ContractType type)
        -> bool override;
    auto GetFuture() -> Future override;
    auto IssueUnitDefinition(
        const std::shared_ptr<const proto::UnitDefinition> unitDefinition,
        const otx::context::Server::ExtraArgs& args) -> bool override;
    void join() override;
    auto PublishContract(const identifier::Nym& id) -> bool override;
    auto PublishContract(const identifier::Server& id) -> bool override;
    auto PublishContract(const identifier::UnitDefinition& id) -> bool override;
    auto RequestAdmin(const String& password) -> bool override;
#if OT_CASH
    auto SendCash(
        const identifier::Nym& recipient,
        const Identifier& workflowID) -> bool override;
#endif
    auto SendMessage(
        const identifier::Nym& recipient,
        const String& message,
        const SetID setID) -> bool override;
    auto SendPeerReply(
        const identifier::Nym& targetNymID,
        const OTPeerReply peerreply,
        const OTPeerRequest peerrequest) -> bool override;
    auto SendPeerRequest(
        const identifier::Nym& targetNymID,
        const OTPeerRequest peerrequest) -> bool override;
    auto SendTransfer(
        const Identifier& sourceAccountID,
        const Identifier& destinationAccountID,
        const Amount amount,
        const String& memo) -> bool override;
    void SetPush(const bool on) override { enable_otx_push_.store(on); }
    void Shutdown() override;
    auto Start(const Type type, const otx::context::Server::ExtraArgs& args)
        -> bool override;
    auto Start(
        const Type type,
        const identifier::UnitDefinition& targetUnitID,
        const otx::context::Server::ExtraArgs& args) -> bool override;
    auto Start(
        const Type type,
        const identifier::Nym& targetNymID,
        const otx::context::Server::ExtraArgs& args) -> bool override;
    auto UpdateAccount(const Identifier& accountID) -> bool override;
#if OT_CASH
    auto WithdrawCash(const Identifier& accountID, const Amount amount)
        -> bool override;
#endif

    ~Operation() override;

private:
    using Promise = std::promise<Result>;
    friend opentxs::Factory;

    enum class Category : int {
        Invalid = 0,
        Basic = 1,
        NymboxPost = 2,
        NymboxPre = 4,
        CreateAccount = 5,
        UpdateAccount = 6,
        Transaction = 7,
    };
    enum class State : int {
        Invalid,
        Idle,
        NymboxPre,
        TransactionNumbers,
        AccountPre,
        Execute,
        AccountPost,
        NymboxPost,
    };
    enum class BoxType : std::int32_t {
        Nymbox = 0,
        Inbox = 1,
        Outbox = 2,
    };

    static const std::map<Type, Category> category_;
    static const std::map<Type, std::size_t> transaction_numbers_;

    const api::client::internal::Manager& api_;
    const OTPasswordPrompt reason_;
    const OTNymID nym_id_;
    const OTServerID server_id_;
    std::atomic<Type> type_;
    std::atomic<State> state_;
    std::atomic<bool> refresh_account_;
    otx::context::Server::ExtraArgs args_;
    std::shared_ptr<Message> message_;
    std::shared_ptr<Message> outmail_message_;
    std::atomic<bool> result_set_;
    std::atomic<bool> enable_otx_push_;
    Promise result_;
    OTNymID target_nym_id_;
    OTServerID target_server_id_;
    OTUnitID target_unit_id_;
    ContractType contract_type_;
    std::shared_ptr<const proto::UnitDefinition> unit_definition_;
    OTIdentifier account_id_;
    OTIdentifier generic_id_;
    Amount amount_;
    OTString memo_;
    bool bool_;
    contact::ContactSectionName claim_section_;
    contact::ContactItemType claim_type_;
    std::shared_ptr<Cheque> cheque_;
    std::shared_ptr<const OTPayment> payment_;
    std::shared_ptr<Ledger> inbox_;
    std::shared_ptr<Ledger> outbox_;
#if OT_CASH
    std::shared_ptr<blind::Purse> purse_;
#endif
    std::set<OTIdentifier> affected_accounts_;
    std::set<OTIdentifier> redownload_accounts_;
    std::set<OTManagedNumber> numbers_;
    std::atomic<std::size_t> error_count_;
    OTPeerReply peer_reply_;
    OTPeerRequest peer_request_;
    SetID set_id_;

    static auto check_future(otx::context::Server::SendFuture& future) -> bool;
    static void set_consensus_hash(
        OTTransaction& transaction,
        const otx::context::Base& context,
        const Account& account,
        const PasswordPrompt& reason);

    auto context() const -> Editor<otx::context::Server>;
    auto evaluate_transaction_reply(
        const Identifier& accountID,
        const Message& reply) const -> bool;
    auto hasContext() const -> bool;
    void update_workflow(
        const Message& request,
        const otx::context::Server::DeliveryResult& result) const;
    void update_workflow_convey_payment(
        const Message& request,
        const otx::context::Server::DeliveryResult& result) const;
    void update_workflow_send_cash(
        const Message& request,
        const otx::context::Server::DeliveryResult& result) const;

    void account_pre();
    void account_post();
    auto construct() -> std::shared_ptr<Message>;
    auto construct_add_claim() -> std::shared_ptr<Message>;
    auto construct_check_nym() -> std::shared_ptr<Message>;
    auto construct_convey_payment() -> std::shared_ptr<Message>;
#if OT_CASH
    auto construct_deposit_cash() -> std::shared_ptr<Message>;
#endif
    auto construct_deposit_cheque() -> std::shared_ptr<Message>;
    auto construct_download_contract() -> std::shared_ptr<Message>;
#if OT_CASH
    auto construct_download_mint() -> std::shared_ptr<Message>;
#endif
    auto construct_get_account_data(const Identifier& accountID)
        -> std::shared_ptr<Message>;
    auto construct_get_transaction_numbers() -> std::shared_ptr<Message>;
    auto construct_issue_unit_definition() -> std::shared_ptr<Message>;
    auto construct_process_inbox(
        const Identifier& accountID,
        const Ledger& payload,
        otx::context::Server& context) -> std::shared_ptr<Message>;
    auto construct_publish_nym() -> std::shared_ptr<Message>;
    auto construct_publish_server() -> std::shared_ptr<Message>;
    auto construct_publish_unit() -> std::shared_ptr<Message>;
    auto construct_register_account() -> std::shared_ptr<Message>;
    auto construct_register_nym() -> std::shared_ptr<Message>;
    auto construct_request_admin() -> std::shared_ptr<Message>;
    auto construct_send_nym_object(
        const PeerObject& object,
        const Nym_p recipient,
        otx::context::Server& context,
        const RequestNumber number = -1) -> std::shared_ptr<Message>;
    auto construct_send_nym_object(
        const PeerObject& object,
        const Nym_p recipient,
        otx::context::Server& context,
        Armored& envelope,
        const RequestNumber number = -1) -> std::shared_ptr<Message>;
    auto construct_send_peer_reply() -> std::shared_ptr<Message>;
    auto construct_send_peer_request() -> std::shared_ptr<Message>;
#if OT_CASH
    auto construct_send_cash() -> std::shared_ptr<Message>;
#endif
    auto construct_send_message() -> std::shared_ptr<Message>;
    auto construct_send_transfer() -> std::shared_ptr<Message>;
#if OT_CASH
    auto construct_withdraw_cash() -> std::shared_ptr<Message>;
#endif
    auto download_account(
        const Identifier& accountID,
        otx::context::Server::DeliveryResult& lastResult) -> std::size_t;
    auto download_accounts(
        const State successState,
        const State failState,
        otx::context::Server::DeliveryResult& lastResult) -> bool;
    auto download_box_receipt(
        const Identifier& accountID,
        const BoxType box,
        const TransactionNumber number) -> bool;
    void evaluate_transaction_reply(
        otx::context::Server::DeliveryResult&& result);
    void execute();
    auto get_account_data(
        const Identifier& accountID,
        std::shared_ptr<Ledger> inbox,
        std::shared_ptr<Ledger> outbox,
        otx::context::Server::DeliveryResult& lastResult) -> bool;
    auto get_receipts(
        const Identifier& accountID,
        std::shared_ptr<Ledger> inbox,
        std::shared_ptr<Ledger> outbox) -> bool;
    auto get_receipts(
        const Identifier& accountID,
        const BoxType type,
        Ledger& box) -> bool;
    void nymbox_post();
    void nymbox_pre();
    auto process_inbox(
        const Identifier& accountID,
        std::shared_ptr<Ledger> inbox,
        std::shared_ptr<Ledger> outbox,
        otx::context::Server::DeliveryResult& lastResult) -> bool;
    void refresh();
    void reset();
    void set_result(otx::context::Server::DeliveryResult&& result);
    auto start(
        const Lock& decisionLock,
        const Type type,
        const otx::context::Server::ExtraArgs& args) -> bool;
    auto state_machine() -> bool;
    void transaction_numbers();

    Operation(
        const api::client::internal::Manager& api,
        const identifier::Nym& nym,
        const identifier::Server& server,
        const PasswordPrompt& reason);
    Operation() = delete;
    Operation(const Operation&) = delete;
    Operation(Operation&&) = delete;
    auto operator=(const Operation&) -> Operation& = delete;
    auto operator=(Operation&&) -> Operation& = delete;
};
}  // namespace opentxs::otx::client::implementation
