// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/otx/consensus/Server.cpp"

#pragma once

#include <atomic>
#include <cstdint>
#include <future>
#include <iosfwd>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "core/StateMachine.hpp"
#include "internal/otx/consensus/Consensus.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/Editor.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Item.hpp"
#include "opentxs/core/Message.hpp"
#include "opentxs/core/OTTransaction.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/network/ServerConnection.hpp"
#include "opentxs/network/zeromq/socket/Push.hpp"
#include "opentxs/otx/LastReplyStatus.hpp"
#include "opentxs/otx/Types.hpp"
#include "opentxs/otx/consensus/Base.hpp"
#include "opentxs/otx/consensus/ManagedNumber.hpp"
#include "opentxs/otx/consensus/Server.hpp"
#include "opentxs/otx/consensus/TransactionStatement.hpp"
#include "opentxs/protobuf/ConsensusEnums.pb.h"
#include "opentxs/protobuf/Context.pb.h"
#include "otx/consensus/Base.hpp"

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

namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace blind
{
class Purse;
}  // namespace blind

namespace identifier
{
class Nym;
class Server;
class UnitDefinition;
}  // namespace identifier

namespace identity
{
class Nym;
}  // namespace identity

namespace network
{
namespace zeromq
{
namespace socket
{
class Publish;
}  // namespace socket
}  // namespace zeromq
}  // namespace network

namespace otx
{
class Reply;
}  // namespace otx

namespace proto
{
class OTXPush;
}  // namespace proto

class Armored;
class Ledger;
class OTPayment;
class PasswordPrompt;
class PeerObject;
class String;
}  // namespace opentxs

namespace zmq = opentxs::network::zeromq;

namespace opentxs::otx::context::implementation
{
class Server final : virtual public internal::Server,
                     public Base,
                     public opentxs::internal::StateMachine
{
public:
    auto Accounts() const -> std::vector<OTIdentifier> final;
    auto AdminPassword() const -> const std::string& final;
    auto AdminAttempted() const -> bool final;
    auto FinalizeServerCommand(Message& command, const PasswordPrompt& reason)
        const -> bool final;
    auto GetContract(const Lock& lock) const -> proto::Context final
    {
        return contract(lock);
    }
    auto HaveAdminPassword() const -> bool final;
    auto HaveSufficientNumbers(const MessageType reason) const -> bool final;
    auto Highest() const -> TransactionNumber final;
    auto isAdmin() const -> bool final;
#if OT_CASH
    auto Purse(const identifier::UnitDefinition& id) const
        -> std::shared_ptr<const blind::Purse> final;
#endif
    auto Revision() const -> std::uint64_t final;
    auto ShouldRename(const std::string& defaultName = "") const -> bool final;
    auto StaleNym() const -> bool final;
    auto Statement(const OTTransaction& owner, const PasswordPrompt& reason)
        const -> std::unique_ptr<Item> final;
    auto Statement(
        const OTTransaction& owner,
        const TransactionNumbers& adding,
        const PasswordPrompt& reason) const -> std::unique_ptr<Item> final;
    auto Statement(
        const TransactionNumbers& adding,
        const TransactionNumbers& without,
        const PasswordPrompt& reason) const
        -> std::unique_ptr<otx::context::TransactionStatement> final;
    auto Type() const -> otx::ConsensusType final;
    auto ValidateContext(const Lock& lock) const -> bool final
    {
        return validate(lock);
    }
    auto Verify(const otx::context::TransactionStatement& statement) const
        -> bool final;
    auto VerifyTentativeNumber(const TransactionNumber& number) const
        -> bool final;

    auto AcceptIssuedNumber(const TransactionNumber& number) -> bool final;
    auto AcceptIssuedNumbers(
        const otx::context::TransactionStatement& statement) -> bool final;
    auto AddTentativeNumber(const TransactionNumber& number) -> bool final;
    auto Connection() -> network::ServerConnection& final;
    auto GetLock() -> std::mutex& final { return lock_; }
    auto InitializeServerCommand(
        const MessageType type,
        const Armored& payload,
        const Identifier& accountID,
        const RequestNumber provided,
        const bool withAcknowledgments = true,
        const bool withNymboxHash = true)
        -> std::pair<RequestNumber, std::unique_ptr<Message>> final;
    auto InitializeServerCommand(
        const MessageType type,
        const identifier::Nym& recipientNymID,
        const RequestNumber provided,
        const bool withAcknowledgments = true,
        const bool withNymboxHash = false)
        -> std::pair<RequestNumber, std::unique_ptr<Message>> final;
    auto InitializeServerCommand(
        const MessageType type,
        const RequestNumber provided,
        const bool withAcknowledgments = true,
        const bool withNymboxHash = false)
        -> std::pair<RequestNumber, std::unique_ptr<Message>> final;
    void Join() const final;
#if OT_CASH
    auto mutable_Purse(
        const identifier::UnitDefinition& id,
        const PasswordPrompt& reason) -> Editor<blind::Purse> final;
#endif
    auto NextTransactionNumber(const MessageType reason)
        -> OTManagedNumber final;
    auto PingNotary(const PasswordPrompt& reason) -> NetworkReplyMessage final;
    auto ProcessNotification(
        const api::client::internal::Manager& client,
        const otx::Reply& notification,
        const PasswordPrompt& reason) -> bool final;
    auto Queue(
        const api::client::internal::Manager& client,
        std::shared_ptr<Message> message,
        const PasswordPrompt& reason,
        const ExtraArgs& args) -> QueueResult final;
    auto Queue(
        const api::client::internal::Manager& client,
        std::shared_ptr<Message> message,
        std::shared_ptr<Ledger> inbox,
        std::shared_ptr<Ledger> outbox,
        std::set<OTManagedNumber>* numbers,
        const PasswordPrompt& reason,
        const ExtraArgs& args) -> QueueResult final;
    auto RefreshNymbox(
        const api::client::internal::Manager& client,
        const PasswordPrompt& reason) -> QueueResult final;
    auto RemoveTentativeNumber(const TransactionNumber& number) -> bool final;
    void ResetThread() final;
    auto Resync(const proto::Context& serialized) -> bool final;
    auto SendMessage(
        const api::client::internal::Manager& client,
        const std::set<OTManagedNumber>& pending,
        otx::context::Server&,
        const Message& message,
        const PasswordPrompt& reason,
        const std::string& label,
        const bool resync) -> NetworkReplyMessage final;
    void SetAdminAttempted() final;
    void SetAdminPassword(const std::string& password) final;
    void SetAdminSuccess() final;
    auto SetHighest(const TransactionNumber& highest) -> bool final;
    void SetPush(const bool on) final { enable_otx_push_.store(on); }
    void SetRevision(const std::uint64_t revision) final;
    auto UpdateHighest(
        const TransactionNumbers& numbers,
        TransactionNumbers& good,
        TransactionNumbers& bad) -> TransactionNumber final;
    auto UpdateRequestNumber(const PasswordPrompt& reason)
        -> RequestNumber final;
    auto UpdateRequestNumber(bool& sendStatus, const PasswordPrompt& reason)
        -> RequestNumber final;
    auto UpdateRequestNumber(Message& command, const PasswordPrompt& reason)
        -> bool final;
    auto UpdateSignature(const Lock& lock, const PasswordPrompt& reason)
        -> bool final
    {
        return update_signature(lock, reason);
    }

    Server(
        const api::client::internal::Manager& api,
        const network::zeromq::socket::Publish& requestSent,
        const network::zeromq::socket::Publish& replyReceived,
        const Nym_p& local,
        const Nym_p& remote,
        const identifier::Server& server,
        network::ServerConnection& connection);
    Server(
        const api::client::internal::Manager& api,
        const network::zeromq::socket::Publish& requestSent,
        const network::zeromq::socket::Publish& replyReceived,
        const proto::Context& serialized,
        const Nym_p& local,
        const Nym_p& remote,
        network::ServerConnection& connection);

    ~Server() final;

private:
    using ReplyNoticeOutcome = std::pair<RequestNumber, Server::DeliveryResult>;
    using ReplyNoticeOutcomes = std::vector<ReplyNoticeOutcome>;

    enum class Exit : bool { Yes = true, Continue = false };
    enum class UpdateHash : bool { Remote = false, Both = true };
    enum class BoxType : std::int64_t {
        Invalid = -1,
        Nymbox = 0,
        Inbox = 1,
        Outbox = 2
    };
    enum class ActionType : bool { ProcessNymbox = true, Normal = false };
    enum class TransactionAttempt : bool { Accepted = true, Rejected = false };

    static const std::string default_node_name_;
    static const std::set<MessageType> do_not_need_request_number_;

    const network::zeromq::socket::Publish& request_sent_;
    const network::zeromq::socket::Publish& reply_received_;
    // WARNING the lifetime of the object pointed to by this member variable
    // has a shorter lifetime than this ServerContext object. Call Join()
    // on all ServerContext objects before allowing the client api to shut down.
    std::atomic<const api::client::internal::Manager*> client_;
    network::ServerConnection& connection_;
    std::mutex message_lock_{};
    std::string admin_password_{""};
    OTFlag admin_attempted_;
    OTFlag admin_success_;
    std::atomic<std::uint64_t> revision_{0};
    std::atomic<TransactionNumber> highest_transaction_number_{0};
    TransactionNumbers tentative_transaction_numbers_{};
    std::atomic<proto::DeliveryState> state_;
    std::atomic<otx::LastReplyStatus> last_status_;
    std::shared_ptr<opentxs::Message> pending_message_;
    ExtraArgs pending_args_;
    std::promise<DeliveryResult> pending_result_;
    std::atomic<bool> pending_result_set_;
    std::atomic<bool> process_nymbox_;
    std::atomic<bool> enable_otx_push_;
    std::atomic<int> failure_counter_;
    std::shared_ptr<Ledger> inbox_;
    std::shared_ptr<Ledger> outbox_;
    std::set<OTManagedNumber>* numbers_;
    OTZMQPushSocket find_nym_;
    OTZMQPushSocket find_server_;
    OTZMQPushSocket find_unit_definition_;

    static auto client(const api::internal::Core& api)
        -> const api::client::internal::Manager&;
    static auto extract_numbers(OTTransaction& input) -> TransactionNumbers;
    static auto get_item_type(OTTransaction& input, itemType& output) -> Exit;
    static auto get_type(const std::int64_t depth) -> BoxType;
    static auto instantiate_message(
        const api::internal::Core& api,
        const std::string& serialized) -> std::unique_ptr<opentxs::Message>;
    static auto need_request_number(const MessageType type) -> bool;
    static void scan_number_set(
        const TransactionNumbers& input,
        TransactionNumber& highest,
        TransactionNumber& lowest);
    static void validate_number_set(
        const TransactionNumbers& input,
        const TransactionNumber limit,
        TransactionNumbers& good,
        TransactionNumbers& bad);

    auto add_item_to_payment_inbox(
        const TransactionNumber number,
        const std::string& payment,
        const PasswordPrompt& reason) const -> bool;
    auto add_item_to_workflow(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Message& transportItem,
        const std::string& item,
        const PasswordPrompt& reason) const -> bool;
    auto add_transaction_to_ledger(
        const TransactionNumber number,
        std::shared_ptr<OTTransaction> transaction,
        Ledger& ledger,
        const PasswordPrompt& reason) const -> bool;
    auto client_nym_id(const Lock& lock) const -> const identifier::Nym& final;
    auto create_instrument_notice_from_peer_object(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Message& message,
        const PeerObject& peerObject,
        const TransactionNumber number,
        const PasswordPrompt& reason) const -> bool;
    auto extract_box_receipt(
        const String& serialized,
        const identity::Nym& signer,
        const identifier::Nym& owner,
        const TransactionNumber target) -> std::shared_ptr<OTTransaction>;
    auto extract_ledger(
        const Armored& armored,
        const Identifier& accountID,
        const identity::Nym& signer) const -> std::unique_ptr<Ledger>;
    auto extract_message(const Armored& armored, const identity::Nym& signer)
        const -> std::unique_ptr<Message>;
    auto extract_original_item(const itemType type, OTTransaction& response)
        const -> std::unique_ptr<Item>;
    auto extract_original_item(const Item& response) const
        -> std::unique_ptr<Item>;
    auto extract_payment_instrument_from_notice(
        const api::internal::Core& api,
        const identity::Nym& theNym,
        std::shared_ptr<OTTransaction> pTransaction,
        const PasswordPrompt& reason) -> std::shared_ptr<OTPayment>;
    auto extract_transfer(const OTTransaction& receipt) const
        -> std::unique_ptr<Item>;
    auto extract_transfer_pending(const OTTransaction& receipt) const
        -> std::unique_ptr<Item>;
    auto extract_transfer_receipt(const OTTransaction& receipt) const
        -> std::unique_ptr<Item>;
    auto finalize_server_command(Message& command, const PasswordPrompt& reason)
        const -> bool;
    auto generate_statement(
        const Lock& lock,
        const TransactionNumbers& adding,
        const TransactionNumbers& without) const
        -> std::unique_ptr<otx::context::TransactionStatement>;
    auto get_instrument(
        const api::internal::Core& api,
        const identity::Nym& theNym,
        Ledger& ledger,
        std::shared_ptr<OTTransaction> pTransaction,
        const PasswordPrompt& reason) -> std::shared_ptr<OTPayment>;
    auto get_instrument_by_receipt_id(
        const api::internal::Core& api,
        const identity::Nym& theNym,
        const TransactionNumber lReceiptId,
        Ledger& ledger,
        const PasswordPrompt& reason) -> std::shared_ptr<OTPayment>;
    auto init_new_account(
        const Identifier& accountID,
        const PasswordPrompt& reason) -> bool;
    auto initialize_server_command(const MessageType type) const
        -> std::unique_ptr<Message>;
    void initialize_server_command(const MessageType type, Message& output)
        const;
    auto is_internal_transfer(const Item& item) const -> bool;
    auto load_account_inbox(const Identifier& accountID) const
        -> std::unique_ptr<Ledger>;
    auto load_or_create_account_recordbox(
        const Identifier& accountID,
        const PasswordPrompt& reason) const -> std::unique_ptr<Ledger>;
    auto load_or_create_payment_inbox(const PasswordPrompt& reason) const
        -> std::unique_ptr<Ledger>;
    void process_accept_pending_reply(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Identifier& accountID,
        const Item& acceptItemReceipt,
        const Message& reply) const;
#if OT_CASH
    auto process_incoming_cash(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const TransactionNumber number,
        const PeerObject& incoming,
        const Message& message) const -> bool;
    void process_incoming_cash_withdrawal(
        const Item& item,
        const PasswordPrompt& reason) const;
#endif
    void process_incoming_instrument(
        const std::shared_ptr<OTTransaction> receipt,
        const PasswordPrompt& reason) const;
    void process_incoming_message(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const OTTransaction& receipt,
        const PasswordPrompt& reason) const;
    auto type() const -> std::string final { return "server"; }
    void verify_blank(
        const Lock& lock,
        OTTransaction& blank,
        TransactionNumbers& output) const;
    void verify_success(
        const Lock& lock,
        OTTransaction& blank,
        TransactionNumbers& output) const;

    auto accept_entire_nymbox(
        const Lock& lock,
        const api::client::internal::Manager& client,
        Ledger& theNymbox,
        Message& output,
        ReplyNoticeOutcomes& notices,
        std::size_t& alreadySeenNotices,
        const PasswordPrompt& reason) -> bool;
    auto accept_issued_number(const Lock& lock, const TransactionNumber& number)
        -> bool;
    auto accept_issued_number(
        const Lock& lock,
        const otx::context::TransactionStatement& statement) -> bool;
    void accept_numbers(
        const Lock& lock,
        OTTransaction& transaction,
        OTTransaction& replyTransaction);
    auto add_tentative_number(const Lock& lock, const TransactionNumber& number)
        -> bool;
    auto attempt_delivery(
        const Lock& contextLock,
        const Lock& messageLock,
        const api::client::internal::Manager& client,
        Message& message,
        const PasswordPrompt& reason) -> NetworkReplyMessage;
    auto harvest_unused(
        const Lock& lock,
        const api::client::internal::Manager& client) -> bool;
    void init_sockets();
    auto initialize_server_command(
        const Lock& lock,
        const MessageType type,
        const RequestNumber provided,
        const bool withAcknowledgments,
        const bool withNymboxHash,
        Message& output) -> RequestNumber;
    auto initialize_server_command(
        const Lock& lock,
        const MessageType type,
        const RequestNumber provided,
        const bool withAcknowledgments,
        const bool withNymboxHash)
        -> std::pair<RequestNumber, std::unique_ptr<Message>>;
    auto make_accept_item(
        const PasswordPrompt& reason,
        const itemType type,
        const OTTransaction& input,
        OTTransaction& acceptTransaction,
        const TransactionNumbers& accept = {}) -> const Item&;
    void need_box_items(
        const api::client::internal::Manager& client,
        const PasswordPrompt& reason);
    void need_nymbox(
        const api::client::internal::Manager& client,
        const PasswordPrompt& reason);
    void need_process_nymbox(
        const api::client::internal::Manager& client,
        const PasswordPrompt& reason);
    auto next_transaction_number(const Lock& lock, const MessageType reason)
        -> OTManagedNumber;
    void pending_send(
        const api::client::internal::Manager& client,
        const PasswordPrompt& reason);
    void process_accept_basket_receipt_reply(
        const Lock& lock,
        const OTTransaction& inboxTransaction);
    void process_accept_cron_receipt_reply(
        const Lock& lock,
        const Identifier& accountID,
        OTTransaction& inboxTransaction);
    void process_accept_final_receipt_reply(
        const Lock& lock,
        const OTTransaction& inboxTransaction);
    void process_accept_item_receipt_reply(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Identifier& accountID,
        const Message& reply,
        const OTTransaction& inboxTransaction);
    auto process_account_data(
        const Lock& lock,
        const Identifier& accountID,
        const String& account,
        const Identifier& inboxHash,
        const String& inbox,
        const Identifier& outboxHash,
        const String& outbox,
        const PasswordPrompt& reason) -> bool;
    auto process_account_push(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const proto::OTXPush& push,
        const PasswordPrompt& reason) -> bool;
    auto process_box_item(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Identifier& accountID,
        const proto::OTXPush& push,
        const PasswordPrompt& reason) -> bool;
    auto process_check_nym_response(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Message& reply) -> bool;
    auto process_get_account_data(
        const Lock& lock,
        const Message& reply,
        const PasswordPrompt& reason) -> bool;
    auto process_get_box_receipt_response(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Message& reply,
        const PasswordPrompt& reason) -> bool;
    auto process_get_box_receipt_response(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Identifier& accountID,
        const std::shared_ptr<OTTransaction> receipt,
        const String& serialized,
        const BoxType type,
        const PasswordPrompt& reason) -> bool;
    auto process_get_market_list_response(
        const Lock& lock,
        const Message& reply) -> bool;
    auto process_get_market_offers_response(
        const Lock& lock,
        const Message& reply) -> bool;
    auto process_get_market_recent_trades_response(
        const Lock& lock,
        const Message& reply) -> bool;
#if OT_CASH
    auto process_get_mint_response(const Lock& lock, const Message& reply)
        -> bool;
#endif
    auto process_get_nym_market_offers_response(
        const Lock& lock,
        const Message& reply) -> bool;
    auto process_get_unit_definition_response(
        const Lock& lock,
        const Message& reply) -> bool;
    auto process_issue_unit_definition_response(
        const Lock& lock,
        const Message& reply,
        const PasswordPrompt& reason) -> bool;
    auto process_get_nymbox_response(
        const Lock& lock,
        const Message& reply,
        const PasswordPrompt& reason) -> bool;
    auto process_notarize_transaction_response(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Message& reply,
        const PasswordPrompt& reason) -> bool;
    auto process_process_box_response(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Message& reply,
        const BoxType inbox,
        const Identifier& accountID,
        const PasswordPrompt& reason) -> bool;
    auto process_process_inbox_response(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Message& reply,
        Ledger& ledger,
        Ledger& responseLedger,
        std::shared_ptr<OTTransaction>& transaction,
        std::shared_ptr<OTTransaction>& replyTransaction,
        const PasswordPrompt& reason) -> bool;
    auto process_process_nymbox_response(
        const Lock& lock,
        const Message& reply,
        Ledger& ledger,
        Ledger& responseLedger,
        std::shared_ptr<OTTransaction>& transaction,
        std::shared_ptr<OTTransaction>& replyTransaction,
        const PasswordPrompt& reason) -> bool;
    auto process_register_account_response(
        const Lock& lock,
        const Message& reply,
        const PasswordPrompt& reason) -> bool;
    auto process_request_admin_response(const Lock& lock, const Message& reply)
        -> bool;
    auto process_register_nym_response(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Message& reply) -> bool;
    auto process_reply(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const std::set<OTManagedNumber>& managed,
        const Message& reply,
        const PasswordPrompt& reason) -> bool;
    void process_response_transaction(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Message& reply,
        OTTransaction& responseTransaction,
        const PasswordPrompt& reason);
    void process_response_transaction_cancel(
        const Lock& lock,
        const Message& reply,
        const itemType type,
        OTTransaction& response);
#if OT_CASH
    void process_response_transaction_cash_deposit(
        Item& replyItem,
        const PasswordPrompt& reason);
#endif
    void process_response_transaction_cheque_deposit(
        const api::client::internal::Manager& client,
        const Identifier& accountID,
        const Message* reply,
        const Item& replyItem,
        const PasswordPrompt& reason);
    void process_response_transaction_cron(
        const Lock& lock,
        const Message& reply,
        const itemType type,
        OTTransaction& response,
        const PasswordPrompt& reason);
    void process_response_transaction_deposit(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Message& reply,
        const itemType type,
        OTTransaction& response,
        const PasswordPrompt& reason);
    void process_response_transaction_exchange_basket(
        const Lock& lock,
        const Message& reply,
        const itemType type,
        OTTransaction& response);
    void process_response_transaction_pay_dividend(
        const Lock& lock,
        const Message& reply,
        const itemType type,
        OTTransaction& response);
    void process_response_transaction_transfer(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Message& reply,
        const itemType type,
        OTTransaction& response);
#if OT_CASH
    void process_response_transaction_withdrawal(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Message& reply,
        const itemType type,
        OTTransaction& response,
        const PasswordPrompt& reason);
#endif
    auto process_unregister_nym_response(
        const Lock& lock,
        const Message& reply,
        const PasswordPrompt& reason) -> bool;
    auto process_unregister_account_response(
        const Lock& lock,
        const Message& reply,
        const PasswordPrompt& reason) -> bool;
    void process_unseen_reply(
        const Lock& lock,
        const api::client::internal::Manager& client,
        const Item& input,
        ReplyNoticeOutcomes& notices,
        const PasswordPrompt& reason);
    using Base::remove_acknowledged_number;
    auto remove_acknowledged_number(const Lock& lock, const Message& reply)
        -> bool;
    auto remove_nymbox_item(
        const Lock& lock,
        const Item& replyItem,
        Ledger& nymbox,
        OTTransaction& transaction,
        const PasswordPrompt& reason) -> bool;
    auto remove_tentative_number(
        const Lock& lock,
        const TransactionNumber& number) -> bool;
    void resolve_queue(
        const Lock& contextLock,
        DeliveryResult&& result,
        const PasswordPrompt& reason,
        const proto::DeliveryState state = proto::DELIVERTYSTATE_ERROR);
    auto resync(const Lock& lock, const proto::Context& serialized) -> bool;
    using Base::serialize;
    auto serialize(const Lock& lock) const -> proto::Context final;
    auto server_nym_id(const Lock& lock) const -> const identifier::Nym& final;
    auto start(
        const Lock& decisionLock,
        const PasswordPrompt& reason,
        const api::client::internal::Manager& client,
        std::shared_ptr<Message> message,
        const ExtraArgs& args,
        const proto::DeliveryState state = proto::DELIVERTYSTATE_PENDINGSEND,
        const ActionType type = ActionType::Normal,
        std::shared_ptr<Ledger> inbox = {},
        std::shared_ptr<Ledger> outbox = {},
        std::set<OTManagedNumber>* numbers = nullptr) -> QueueResult;
    auto state_machine() noexcept -> bool;
    auto statement(
        const Lock& lock,
        const OTTransaction& owner,
        const TransactionNumbers& adding,
        const PasswordPrompt& reason) const -> std::unique_ptr<Item>;
    auto update_highest(
        const Lock& lock,
        const TransactionNumbers& numbers,
        TransactionNumbers& good,
        TransactionNumbers& bad) -> TransactionNumber;
    auto update_nymbox_hash(
        const Lock& lock,
        const Message& reply,
        const UpdateHash which = UpdateHash::Remote) -> bool;
    auto update_remote_hash(const Lock& lock, const Message& reply)
        -> OTIdentifier;
    auto update_request_number(
        const PasswordPrompt& reason,
        const Lock& contextLock,
        const Lock& messageLock,
        bool& sendStatus) -> RequestNumber;
    auto update_request_number(
        const PasswordPrompt& reason,
        const Lock& lock,
        Message& command) -> bool;
    void update_state(
        const Lock& contextLock,
        const proto::DeliveryState state,
        const PasswordPrompt& reason,
        const otx::LastReplyStatus status = otx::LastReplyStatus::Invalid);
    auto verify_tentative_number(
        const Lock& lock,
        const TransactionNumber& number) const -> bool;

    Server() = delete;
    Server(const Server&) = delete;
    Server(Server&&) = delete;
    auto operator=(const Server&) -> Server& = delete;
    auto operator=(Server&&) -> Server& = delete;
};
}  // namespace opentxs::otx::context::implementation
