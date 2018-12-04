// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

namespace zmq = opentxs::network::zeromq;

namespace opentxs::rpc::implementation
{
class RPC final : virtual public rpc::internal::RPC, Lockable
{
public:
    proto::RPCResponse Process(const proto::RPCCommand& command) const override;

    ~RPC() = default;

private:
    friend opentxs::Factory;

    using Args = const ::google::protobuf::RepeatedPtrField<
        ::opentxs::proto::APIArgument>;
    using TaskID = std::string;
    using Future = api::client::OTX::Future;
    using Result = api::client::OTX::Result;
    using Finish =
        std::function<void(const Result& result, proto::TaskComplete& output)>;
    using TaskData = std::tuple<Future, Finish, OTNymID>;

    const api::Native& ot_;
    mutable std::mutex task_lock_;
    mutable std::map<TaskID, TaskData> queued_tasks_;
    const OTZMQListenCallback task_callback_;
    const OTZMQListenCallback push_callback_;
    const OTZMQPullSocket push_receiver_;
    const OTZMQPublishSocket rpc_publisher_;
    const OTZMQSubscribeSocket task_subscriber_;

    static void add_output_status(
        proto::RPCResponse& output,
        proto::RPCResponseCode code);
    static void add_output_status(
        proto::TaskComplete& output,
        proto::RPCResponseCode code);
    static void add_output_identifier(
        const std::string& id,
        proto::RPCResponse& output);
    static void add_output_identifier(
        const std::string& id,
        proto::TaskComplete& output);
    static void add_output_task(
        proto::RPCResponse& output,
        const std::string& taskid);
    static ArgList get_args(const Args& serialized);
    static std::size_t get_index(std::int32_t instance);
    static proto::RPCResponse init(const proto::RPCCommand& command);
    static proto::RPCResponse invalid_command(const proto::RPCCommand& command);
    static proto::AccountEventType storagebox_to_accounteventtype(
        StorageBox storagebox);

    proto::RPCResponse accept_pending_payments(
        const proto::RPCCommand& command) const;
    proto::RPCResponse add_claim(const proto::RPCCommand& command) const;
    proto::RPCResponse add_contact(const proto::RPCCommand& command) const;
    proto::RPCResponse create_account(const proto::RPCCommand& command) const;
    proto::RPCResponse create_compatible_account(
        const proto::RPCCommand& command) const;
    proto::RPCResponse create_issuer_account(
        const proto::RPCCommand& command) const;
    proto::RPCResponse create_nym(const proto::RPCCommand& command) const;
    proto::RPCResponse create_unit_definition(
        const proto::RPCCommand& command) const;
    proto::RPCResponse delete_claim(const proto::RPCCommand& command) const;
    void evaluate_deposit_payment(
        const api::client::Manager& client,
        const api::client::OTX::Result& result,
        proto::TaskComplete& output) const;
    void evaluate_move_funds(
        const api::client::Manager& client,
        const api::client::OTX::Result& result,
        proto::RPCResponse& output) const;
    template <typename T>
    void evaluate_register_account(
        const api::client::OTX::Result& result,
        T& output) const;
    template <typename T>
    void evaluate_register_nym(
        const api::client::OTX::Result& result,
        T& output) const;
    void evaluate_send_payment_cheque(
        const api::client::OTX::Result& result,
        proto::TaskComplete& output) const;
    void evaluate_send_payment_transfer(
        const api::client::Manager& client,
        const api::client::OTX::Result& result,
        proto::RPCResponse& output) const;
    template <typename T>
    void evaluate_transaction_reply(
        const api::client::Manager& client,
        const Message& reply,
        T& output,
        const proto::RPCResponseCode code =
            proto::RPCRESPONSE_TRANSACTION_FAILED) const;
    const api::client::Manager* get_client(std::int32_t instance) const;
    proto::RPCResponse get_account_activity(
        const proto::RPCCommand& command) const;
    proto::RPCResponse get_account_balance(
        const proto::RPCCommand& command) const;
    proto::RPCResponse get_compatible_accounts(
        const proto::RPCCommand& command) const;
    proto::RPCResponse get_nyms(const proto::RPCCommand& command) const;
    proto::RPCResponse get_pending_payments(
        const proto::RPCCommand& command) const;
    proto::RPCResponse get_seeds(const proto::RPCCommand& command) const;
    const api::server::Manager* get_server(std::int32_t instance) const;
    proto::RPCResponse get_server_admin_nym(
        const proto::RPCCommand& command) const;
    proto::RPCResponse get_server_contracts(
        const proto::RPCCommand& command) const;
    proto::RPCResponse get_server_password(
        const proto::RPCCommand& command) const;
    const api::Core& get_session(std::int32_t instance) const;
    proto::RPCResponse get_transaction_data(
        const proto::RPCCommand& command) const;
    proto::RPCResponse get_unit_definitions(
        const proto::RPCCommand& command) const;
    proto::RPCResponse get_workflow(const proto::RPCCommand& command) const;
    bool immediate_create_account(
        const api::client::Manager& client,
        const identifier::Nym& owner,
        const identifier::Server& notary,
        const identifier::UnitDefinition& unit) const;
    bool immediate_register_issuer_account(
        const api::client::Manager& client,
        const identifier::Nym& owner,
        const identifier::Server& notary) const;
    bool immediate_register_nym(
        const api::client::Manager& client,
        const identifier::Server& notary) const;
    proto::RPCResponse import_seed(const proto::RPCCommand& command) const;
    proto::RPCResponse import_server_contract(
        const proto::RPCCommand& command) const;
    bool is_client_session(std::int32_t instance) const;
    bool is_server_session(std::int32_t instance) const;
    bool is_session_valid(std::int32_t instance) const;
    proto::RPCResponse list_accounts(const proto::RPCCommand& command) const;
    proto::RPCResponse list_contacts(const proto::RPCCommand& command) const;
    proto::RPCResponse list_client_sessions(
        const proto::RPCCommand& command) const;
    proto::RPCResponse list_seeds(const proto::RPCCommand& command) const;
    proto::RPCResponse list_nyms(const proto::RPCCommand& command) const;
    proto::RPCResponse list_server_contracts(
        const proto::RPCCommand& command) const;
    proto::RPCResponse list_server_sessions(
        const proto::RPCCommand& command) const;
    proto::RPCResponse list_unit_definitions(
        const proto::RPCCommand& command) const;
    proto::RPCResponse lookup_account_id(
        const proto::RPCCommand& command) const;
    proto::RPCResponse move_funds(const proto::RPCCommand& command) const;
    void queue_task(
        const identifier::Nym& nymID,
        const std::string taskID,
        Finish&& finish,
        Future&& future,
        proto::RPCResponse& output) const;
    proto::RPCResponse register_nym(const proto::RPCCommand& command) const;
    proto::RPCResponse rename_account(const proto::RPCCommand& command) const;
    proto::RPCResponse send_payment(const proto::RPCCommand& command) const;
    proto::RPCResponse start_client(const proto::RPCCommand& command) const;
    proto::RPCResponse start_server(const proto::RPCCommand& command) const;

    void task_handler(const zmq::Message& message);

    RPC(const api::Native& native);
    RPC() = delete;
    RPC(const RPC&) = delete;
    RPC(RPC&&) = delete;
    RPC& operator=(const RPC&) = delete;
    RPC& operator=(RPC&&) = delete;
};
}  // namespace opentxs::rpc::implementation
