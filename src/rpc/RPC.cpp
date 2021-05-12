// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"    // IWYU pragma: associated
#include "1_Internal.hpp"  // IWYU pragma: associated
#include "rpc/RPC.tpp"     // IWYU pragma: associated

#include <algorithm>
#include <cstdint>
#include <functional>
#include <future>
#include <iosfwd>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include "2_Factory.hpp"
#include "internal/api/client/Client.hpp"
#include "internal/contact/Contact.hpp"
#include "opentxs/Exclusive.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Shared.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/HDSeed.hpp"
#include "opentxs/api/Primitives.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/client/OTX.hpp"
#include "opentxs/api/client/PaymentWorkflowState.hpp"
#include "opentxs/api/client/PaymentWorkflowType.hpp"
#include "opentxs/api/client/Workflow.hpp"
#include "opentxs/api/server/Manager.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/client/NymData.hpp"
#include "opentxs/client/OT_API.hpp"
#include "opentxs/contact/Contact.hpp"
#include "opentxs/contact/ContactItemType.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/Armored.hpp"
#include "opentxs/core/Cheque.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Ledger.hpp"
#include "opentxs/core/Lockable.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/OTTransaction.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/crypto/Language.hpp"
#include "opentxs/crypto/SeedStyle.hpp"
#include "opentxs/ext/OTPayment.hpp"  // IWYU pragma: keep
#include "opentxs/identity/Nym.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/ListenCallback.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/socket/Publish.hpp"
#include "opentxs/network/zeromq/socket/Pull.hpp"
#include "opentxs/network/zeromq/socket/Sender.tpp"
#include "opentxs/network/zeromq/socket/Socket.hpp"
#include "opentxs/network/zeromq/socket/Subscribe.hpp"
#include "opentxs/protobuf/APIArgument.pb.h"
#include "opentxs/protobuf/AcceptPendingPayment.pb.h"
#include "opentxs/protobuf/AccountEvent.pb.h"
#include "opentxs/protobuf/AddClaim.pb.h"
#include "opentxs/protobuf/AddContact.pb.h"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/ContactItem.pb.h"
#include "opentxs/protobuf/CreateInstrumentDefinition.pb.h"
#include "opentxs/protobuf/CreateNym.pb.h"
#include "opentxs/protobuf/GetWorkflow.pb.h"
#include "opentxs/protobuf/HDSeed.pb.h"
#include "opentxs/protobuf/ModifyAccount.pb.h"
#include "opentxs/protobuf/MoveFunds.pb.h"
#include "opentxs/protobuf/Nym.pb.h"
#include "opentxs/protobuf/PaymentEvent.pb.h"
#include "opentxs/protobuf/PaymentWorkflow.pb.h"
#include "opentxs/protobuf/RPCCommand.pb.h"
#include "opentxs/protobuf/RPCEnums.pb.h"
#include "opentxs/protobuf/RPCPush.pb.h"
#include "opentxs/protobuf/RPCResponse.pb.h"
#include "opentxs/protobuf/RPCStatus.pb.h"
#include "opentxs/protobuf/RPCTask.pb.h"
#include "opentxs/protobuf/ServerContract.pb.h"
#include "opentxs/protobuf/SessionData.pb.h"
#include "opentxs/protobuf/TaskComplete.pb.h"
#include "opentxs/protobuf/UnitDefinition.pb.h"
#include "opentxs/protobuf/verify/RPCCommand.hpp"
#include "opentxs/rpc/CommandType.hpp"
#include "opentxs/rpc/ResponseCode.hpp"
#include "opentxs/rpc/request/Base.hpp"
#include "opentxs/rpc/response/Base.hpp"
#include "rpc/RPC.hpp"
#include "rpc/response/Invalid.hpp"

#define ACCOUNTEVENT_VERSION 2
#define RPCTASK_VERSION 1
#if OT_CRYPTO_WITH_BIP32
#define SEED_VERSION 1
#endif  // OT_CRYPTO_WITH_BIP32
#define SESSION_DATA_VERSION 1
#define RPCPUSH_VERSION 3
#define TASKCOMPLETE_VERSION 2

#define CHECK_INPUT(field, error)                                              \
    if (0 == command.field().size()) {                                         \
        add_output_status(output, error);                                      \
                                                                               \
        return output;                                                         \
    }

#define CHECK_OWNER()                                                          \
    if (false == session.Wallet().IsLocalNym(command.owner())) {               \
        add_output_status(output, proto::RPCRESPONSE_NYM_NOT_FOUND);           \
                                                                               \
        return output;                                                         \
    }                                                                          \
                                                                               \
    const auto ownerID = identifier::Nym::Factory(command.owner());

#define INIT() auto output = init(command);

#define INIT_SESSION()                                                         \
    INIT();                                                                    \
                                                                               \
    if (false == is_session_valid(command.session())) {                        \
        add_output_status(output, proto::RPCRESPONSE_BAD_SESSION);             \
                                                                               \
        return output;                                                         \
    }                                                                          \
                                                                               \
    [[maybe_unused]] auto& session = get_session(command.session());           \
    [[maybe_unused]] auto reason = session.Factory().PasswordPrompt("RPC");

#define INIT_CLIENT_ONLY()                                                     \
    INIT_SESSION();                                                            \
                                                                               \
    if (false == is_client_session(command.session())) {                       \
        add_output_status(output, proto::RPCRESPONSE_INVALID);                 \
                                                                               \
        return output;                                                         \
    }                                                                          \
                                                                               \
    const auto pClient = get_client(command.session());                        \
                                                                               \
    OT_ASSERT(nullptr != pClient)                                              \
                                                                               \
    [[maybe_unused]] const auto& client = *pClient;

#define INIT_SERVER_ONLY()                                                     \
    INIT_SESSION();                                                            \
                                                                               \
    if (false == is_server_session(command.session())) {                       \
        add_output_status(output, proto::RPCRESPONSE_INVALID);                 \
                                                                               \
        return output;                                                         \
    }                                                                          \
                                                                               \
    const auto pServer = get_server(command.session());                        \
                                                                               \
    OT_ASSERT(nullptr != pServer)                                              \
                                                                               \
    [[maybe_unused]] const auto& server = *pServer;

#define INIT_OTX(a, ...)                                                       \
    api::client::OTX::Result result{otx::LastReplyStatus::NotSent, nullptr};   \
    [[maybe_unused]] const auto& [status, pReply] = result;                    \
    [[maybe_unused]] auto [taskID, future] = client.OTX().a(__VA_ARGS__);      \
    [[maybe_unused]] const auto ready = (0 != taskID);

namespace opentxs
{
auto Factory::RPC(const api::Context& native) -> rpc::internal::RPC*
{
    return new rpc::implementation::RPC(native);
}
}  // namespace opentxs

namespace opentxs::rpc::implementation
{
static const std::map<VersionNumber, VersionNumber> StatusVersionMap{
    {1, 1},
    {2, 2},
    {3, 2},
};

RPC::RPC(const api::Context& native)
    : Lockable()
    , ot_(native)
    , task_lock_()
    , queued_tasks_()
    , task_callback_(zmq::ListenCallback::Factory(
          std::bind(&RPC::task_handler, this, std::placeholders::_1)))
    , push_callback_(zmq::ListenCallback::Factory([&](const zmq::Message& in) {
        rpc_publisher_->Send(OTZMQMessage{in});
    }))
    , push_receiver_(ot_.ZMQ().PullSocket(
          push_callback_,
          zmq::socket::Socket::Direction::Bind))
    , rpc_publisher_(ot_.ZMQ().PublishSocket())
    , task_subscriber_(ot_.ZMQ().SubscribeSocket(task_callback_))
{
    auto bound = push_receiver_->Start(
        ot_.ZMQ().BuildEndpoint("rpc/push/internal", -1, 1));

    OT_ASSERT(bound)

    bound = rpc_publisher_->Start(ot_.ZMQ().BuildEndpoint("rpc/push", -1, 1));

    OT_ASSERT(bound)
}

auto RPC::accept_pending_payments(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_CLIENT_ONLY();
    CHECK_INPUT(acceptpendingpayment, proto::RPCRESPONSE_INVALID);

    for (auto acceptpendingpayment : command.acceptpendingpayment()) {
        const auto destinationaccountID =
            Identifier::Factory(acceptpendingpayment.destinationaccount());
        const auto workflowID =
            Identifier::Factory(acceptpendingpayment.workflow());
        const auto nymID = client.Storage().AccountOwner(destinationaccountID);
        const auto paymentWorkflow =
            client.Workflow().LoadWorkflow(nymID, workflowID);

        if (false == bool(paymentWorkflow)) {
            LogDetail(OT_METHOD)(__FUNCTION__)(": Invalid workflow ")(
                workflowID)
                .Flush();
            add_output_task(output, "");
            add_output_status(output, proto::RPCRESPONSE_WORKFLOW_NOT_FOUND);

            continue;
        }

        std::shared_ptr<const OTPayment> payment{};

        switch (api::client::internal::translate(paymentWorkflow->type())) {
            case api::client::PaymentWorkflowType::IncomingCheque:
            case api::client::PaymentWorkflowType::IncomingInvoice: {
                auto chequeState =
                    opentxs::api::client::Workflow::InstantiateCheque(
                        client, *paymentWorkflow);
                const auto& [state, cheque] = chequeState;

                if (false == bool(cheque)) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Unable to load cheque from workflow")
                        .Flush();
                    add_output_task(output, "");
                    add_output_status(
                        output, proto::RPCRESPONSE_CHEQUE_NOT_FOUND);

                    continue;
                }

                payment.reset(
                    client.Factory().Payment(*cheque, reason).release());
            } break;
            case api::client::PaymentWorkflowType::OutgoingCheque:
            case api::client::PaymentWorkflowType::OutgoingInvoice:
            case api::client::PaymentWorkflowType::Error:
            default: {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Unsupported workflow type")
                    .Flush();
                add_output_task(output, "");
                add_output_status(output, proto::RPCRESPONSE_ERROR);

                continue;
            }
        }

        if (false == bool(payment)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to instantiate payment")
                .Flush();
            add_output_task(output, "");
            add_output_status(output, proto::RPCRESPONSE_PAYMENT_NOT_FOUND);

            continue;
        }

        INIT_OTX(DepositPayment, nymID, destinationaccountID, payment);

        if (false == ready) {
            add_output_task(output, "");
            add_output_status(output, proto::RPCRESPONSE_START_TASK_FAILED);
        } else {
            queue_task(
                nymID,
                std::to_string(taskID),
                [&](const auto& in, auto& out) -> void {
                    evaluate_deposit_payment(client, in, out);
                },
                std::move(future),
                output);
        }
    }

    return output;
}

auto RPC::add_claim(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_SESSION();
    CHECK_OWNER();
    CHECK_INPUT(claim, proto::RPCRESPONSE_INVALID);

    auto nymdata = session.Wallet().mutable_Nym(
        identifier::Nym::Factory(command.owner()), reason);

    for (const auto& addclaim : command.claim()) {
        const auto& contactitem = addclaim.item();
        std::set<std::uint32_t> attributes(
            contactitem.attribute().begin(), contactitem.attribute().end());
        auto claim = Claim(
            contactitem.id(),
            addclaim.sectionversion(),
            contactitem.type(),
            contactitem.value(),
            contactitem.start(),
            contactitem.end(),
            attributes);

        if (nymdata.AddClaim(claim, reason)) {
            add_output_status(output, proto::RPCRESPONSE_SUCCESS);
        } else {
            add_output_status(output, proto::RPCRESPONSE_ADD_CLAIM_FAILED);
        }
    }

    return output;
}

auto RPC::add_contact(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_CLIENT_ONLY();
    CHECK_INPUT(addcontact, proto::RPCRESPONSE_INVALID);

    for (const auto& addContact : command.addcontact()) {
        const auto contact = client.Contacts().NewContact(
            addContact.label(),
            identifier::Nym::Factory(addContact.nymid()),
            client.Factory().PaymentCode(addContact.paymentcode()));

        if (false == bool(contact)) {
            add_output_status(output, proto::RPCRESPONSE_ADD_CONTACT_FAILED);
        } else {
            output.add_identifier(contact->ID().str());
            add_output_status(output, proto::RPCRESPONSE_SUCCESS);
        }
    }

    if (0 == output.identifier_size()) {
        add_output_status(output, proto::RPCRESPONSE_NONE);
    }

    return output;
}

void RPC::add_output_identifier(
    const std::string& id,
    proto::TaskComplete& output)
{
    output.set_identifier(id);
}

void RPC::add_output_identifier(
    const std::string& id,
    proto::RPCResponse& output)
{
    output.add_identifier(id);
}

void RPC::add_output_status(
    proto::RPCResponse& output,
    proto::RPCResponseCode code)
{
    auto& status = *output.add_status();
    status.set_version(StatusVersionMap.at(output.version()));
    status.set_index(output.status_size() - 1);
    status.set_code(code);
}

void RPC::add_output_status(
    proto::TaskComplete& output,
    proto::RPCResponseCode code)
{
    output.set_code(code);
}

void RPC::add_output_task(proto::RPCResponse& output, const std::string& taskid)
{
    auto& task = *output.add_task();
    task.set_version(RPCTASK_VERSION);
    task.set_index(output.task_size() - 1);
    task.set_id(taskid);
}

auto RPC::client_session(const request::Base& command) const noexcept(false)
    -> const api::client::internal::Manager&
{
    const auto session = command.Session();

    if (false == is_session_valid(session)) {

        throw std::runtime_error{"invalid session"};
    }

    if (is_client_session(session)) {

        return dynamic_cast<const api::client::internal::Manager&>(
            ot_.Client(static_cast<int>(get_index(session))));
    } else {

        throw std::runtime_error{"expected client session"};
    }
}

auto RPC::create_account(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_CLIENT_ONLY();
    CHECK_OWNER();

    const auto notaryID = identifier::Server::Factory(command.notary());
    const auto unitID = identifier::UnitDefinition::Factory(command.unit());
    std::string label{};

    if (0 < command.identifier_size()) { label = command.identifier(0); }

    INIT_OTX(RegisterAccount, ownerID, notaryID, unitID, label);

    if (false == ready) {
        add_output_status(output, proto::RPCRESPONSE_ERROR);

        return output;
    }

    if (immediate_create_account(client, ownerID, notaryID, unitID)) {
        evaluate_register_account(future.get(), output);
    } else {
        queue_task(
            ownerID,
            std::to_string(taskID),
            [&](const auto& in, auto& out) -> void {
                evaluate_register_account(in, out);
            },
            std::move(future),
            output);
    }

    return output;
}

auto RPC::create_compatible_account(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_CLIENT_ONLY();
    CHECK_OWNER();
    CHECK_INPUT(identifier, proto::RPCRESPONSE_INVALID);

    const auto workflowID = Identifier::Factory(command.identifier(0));
    const auto pPaymentWorkflow =
        client.Workflow().LoadWorkflow(ownerID, workflowID);

    if (false == bool(pPaymentWorkflow)) {
        add_output_status(output, proto::RPCRESPONSE_WORKFLOW_NOT_FOUND);

        return output;
    }

    const auto& workflow = *pPaymentWorkflow;
    auto notaryID = identifier::Server::Factory();
    auto unitID = identifier::UnitDefinition::Factory();

    switch (api::client::internal::translate(workflow.type())) {
        case api::client::PaymentWorkflowType::IncomingCheque: {
            auto chequeState =
                opentxs::api::client::Workflow::InstantiateCheque(
                    client, workflow);
            const auto& [state, cheque] = chequeState;

            if (false == bool(cheque)) {
                add_output_status(output, proto::RPCRESPONSE_CHEQUE_NOT_FOUND);

                return output;
            }

            notaryID->SetString(cheque->GetNotaryID().str());
            unitID->SetString(cheque->GetInstrumentDefinitionID().str());
        } break;
        default: {
            add_output_status(output, proto::RPCRESPONSE_INVALID);

            return output;
        }
    }

    INIT_OTX(RegisterAccount, ownerID, notaryID, unitID, "");

    if (false == ready) {
        add_output_status(output, proto::RPCRESPONSE_ERROR);

        return output;
    }

    if (immediate_create_account(client, ownerID, notaryID, unitID)) {
        evaluate_register_account(future.get(), output);
    } else {
        queue_task(
            ownerID,
            std::to_string(taskID),
            [&](const auto& in, auto& out) -> void {
                evaluate_register_account(in, out);
            },
            std::move(future),
            output);
    }

    return output;
}

auto RPC::create_issuer_account(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_CLIENT_ONLY();
    CHECK_OWNER();

    std::string label{};
    auto notaryID = identifier::Server::Factory(command.notary());
    auto unitID = identifier::UnitDefinition::Factory(command.unit());

    if (0 < command.identifier_size()) { label = command.identifier(0); }

    try {
        const auto unitdefinition = client.Wallet().UnitDefinition(unitID);

        if (ownerID != unitdefinition->Nym()->ID()) {
            add_output_status(
                output, proto::RPCRESPONSE_UNITDEFINITION_NOT_FOUND);

            return output;
        }
    } catch (...) {
        add_output_status(output, proto::RPCRESPONSE_UNITDEFINITION_NOT_FOUND);

        return output;
    }

    const auto account = client.Wallet().IssuerAccount(unitID);

    if (account) {
        add_output_status(output, proto::RPCRESPONSE_UNNECESSARY);

        return output;
    }

    INIT_OTX(
        IssueUnitDefinition,
        ownerID,
        notaryID,
        unitID,
        contact::ContactItemType::Error,
        label);

    if (false == ready) {
        add_output_status(output, proto::RPCRESPONSE_ERROR);

        return output;
    }

    if (immediate_register_issuer_account(client, ownerID, notaryID)) {
        evaluate_register_account(future.get(), output);
    } else {
        queue_task(
            ownerID,
            std::to_string(taskID),
            [&](const auto& in, auto& out) -> void {
                evaluate_register_account(in, out);
            },
            std::move(future),
            output);
    }

    return output;
}

auto RPC::create_nym(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_CLIENT_ONLY();

    const auto& createnym = command.createnym();
    const auto pNym = client.Wallet().Nym(
        reason,
        createnym.name(),
        {createnym.seedid(), createnym.index()},
        contact::internal::translate(createnym.type()));

    if (false == bool(pNym)) {
        add_output_status(output, proto::RPCRESPONSE_CREATE_NYM_FAILED);

        return output;
    }

    const auto& nym = *pNym;

    if (0 < createnym.claims_size()) {
        auto nymdata = client.Wallet().mutable_Nym(nym.ID(), reason);

        for (const auto& addclaim : createnym.claims()) {
            const auto& contactitem = addclaim.item();
            std::set<std::uint32_t> attributes(
                contactitem.attribute().begin(), contactitem.attribute().end());
            auto claim = Claim(
                contactitem.id(),
                addclaim.sectionversion(),
                contactitem.type(),
                contactitem.value(),
                contactitem.start(),
                contactitem.end(),
                attributes);
            nymdata.AddClaim(claim, reason);
        }
    }

    output.add_identifier(nym.ID().str());
    add_output_status(output, proto::RPCRESPONSE_SUCCESS);

    return output;
}

auto RPC::create_unit_definition(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_SESSION();
    CHECK_OWNER();

    const auto& createunit = command.createunit();

    try {
        const auto unitdefinition = session.Wallet().UnitDefinition(
            command.owner(),
            createunit.primaryunitname(),
            createunit.name(),
            createunit.symbol(),
            createunit.terms(),
            createunit.tla(),
            createunit.power(),
            createunit.fractionalunitname(),
            contact::internal::translate(createunit.unitofaccount()),
            reason);

        output.add_identifier(unitdefinition->ID()->str());
        add_output_status(output, proto::RPCRESPONSE_SUCCESS);
    } catch (...) {
        add_output_status(
            output, proto::RPCRESPONSE_CREATE_UNITDEFINITION_FAILED);
    }

    return output;
}

auto RPC::delete_claim(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_SESSION();
    CHECK_OWNER();
    CHECK_INPUT(identifier, proto::RPCRESPONSE_INVALID);

    auto nymdata = session.Wallet().mutable_Nym(

        identifier::Nym::Factory(command.owner()), reason);

    for (const auto& id : command.identifier()) {
        auto deleted = nymdata.DeleteClaim(Identifier::Factory(id), reason);

        if (deleted) {
            add_output_status(output, proto::RPCRESPONSE_SUCCESS);
        } else {
            add_output_status(output, proto::RPCRESPONSE_DELETE_CLAIM_FAILED);
        }
    }

    return output;
}

void RPC::evaluate_deposit_payment(
    const api::client::internal::Manager& client,
    const api::client::OTX::Result& result,
    proto::TaskComplete& output) const
{
    // TODO use structured binding
    // const auto& [status, pReply] = result;
    const auto& status = std::get<0>(result);
    const auto& pReply = std::get<1>(result);

    if (otx::LastReplyStatus::NotSent == status) {
        add_output_status(output, proto::RPCRESPONSE_ERROR);
    } else if (otx::LastReplyStatus::Unknown == status) {
        add_output_status(output, proto::RPCRESPONSE_BAD_SERVER_RESPONSE);
    } else if (otx::LastReplyStatus::MessageFailed == status) {
        add_output_status(output, proto::RPCRESPONSE_TRANSACTION_FAILED);
    } else if (otx::LastReplyStatus::MessageSuccess == status) {
        OT_ASSERT(pReply);

        const auto& reply = *pReply;
        evaluate_transaction_reply(client, reply, output);
    }
}

void RPC::evaluate_move_funds(
    const api::client::internal::Manager& client,
    const api::client::OTX::Result& result,
    proto::RPCResponse& output) const
{
    // TODO use structured binding
    // const auto& [status, pReply] = result;
    const auto& status = std::get<0>(result);
    const auto& pReply = std::get<1>(result);

    if (otx::LastReplyStatus::NotSent == status) {
        add_output_status(output, proto::RPCRESPONSE_ERROR);
    } else if (otx::LastReplyStatus::Unknown == status) {
        add_output_status(output, proto::RPCRESPONSE_BAD_SERVER_RESPONSE);
    } else if (otx::LastReplyStatus::MessageFailed == status) {
        add_output_status(output, proto::RPCRESPONSE_MOVE_FUNDS_FAILED);
    } else if (otx::LastReplyStatus::MessageSuccess == status) {
        OT_ASSERT(pReply);

        const auto& reply = *pReply;
        evaluate_transaction_reply(client, reply, output);
    }
}

auto RPC::evaluate_send_payment_cheque(
    const api::client::OTX::Result& result,
    proto::TaskComplete& output) const noexcept -> void
{
    const auto& [status, pReply] = result;

    if (otx::LastReplyStatus::NotSent == status) {
        add_output_status(output, proto::RPCRESPONSE_ERROR);
    } else if (otx::LastReplyStatus::Unknown == status) {
        add_output_status(output, proto::RPCRESPONSE_BAD_SERVER_RESPONSE);
    } else if (otx::LastReplyStatus::MessageFailed == status) {
        add_output_status(output, proto::RPCRESPONSE_SEND_PAYMENT_FAILED);
    } else if (otx::LastReplyStatus::MessageSuccess == status) {
        add_output_status(output, proto::RPCRESPONSE_SUCCESS);
    }
}

auto RPC::evaluate_send_payment_transfer(
    const api::client::Manager& api,
    const api::client::OTX::Result& result,
    proto::TaskComplete& output) const noexcept -> void
{
    const auto& [status, pReply] = result;

    if (otx::LastReplyStatus::NotSent == status) {
        add_output_status(output, proto::RPCRESPONSE_ERROR);
    } else if (otx::LastReplyStatus::Unknown == status) {
        add_output_status(output, proto::RPCRESPONSE_BAD_SERVER_RESPONSE);
    } else if (otx::LastReplyStatus::MessageFailed == status) {
        add_output_status(output, proto::RPCRESPONSE_SEND_PAYMENT_FAILED);
    } else if (otx::LastReplyStatus::MessageSuccess == status) {
        OT_ASSERT(pReply);

        const auto& reply = *pReply;

        evaluate_transaction_reply(api, reply, output);
    }
}

auto RPC::evaluate_transaction_reply(
    const api::client::Manager& api,
    const Message& reply) const noexcept -> bool
{
    auto success{true};
    const auto notaryID = api.Factory().ServerID(reply.m_strNotaryID);
    const auto nymID = api.Factory().NymID(reply.m_strNymID);
    const auto accountID = api.Factory().Identifier(reply.m_strAcctID);
    const bool transaction =
        reply.m_strCommand->Compare("notarizeTransactionResponse") ||
        reply.m_strCommand->Compare("processInboxResponse") ||
        reply.m_strCommand->Compare("processNymboxResponse");

    if (transaction) {
        if (const auto sLedger = String::Factory(reply.m_ascPayload);
            sLedger->Exists()) {
            if (auto ledger{api.Factory().Ledger(nymID, accountID, notaryID)};
                ledger->LoadContractFromString(sLedger)) {
                if (ledger->GetTransactionCount() > 0) {
                    for (const auto& [key, value] :
                         ledger->GetTransactionMap()) {
                        if (false == bool(value)) {
                            success = false;
                            break;
                        } else {
                            success &= value->GetSuccess();
                        }
                    }
                } else {
                    success = false;
                }
            } else {
                success = false;
            }
        } else {
            success = false;
        }
    } else {
        success = false;
    }

    return success;
}

auto RPC::get_args(const Args& serialized) -> ArgList
{
    ArgList output{};

    for (const auto& arg : serialized) {
        auto& row = output[arg.key()];

        for (const auto& value : arg.value()) { row.emplace(value); }
    }

    return output;
}

auto RPC::get_client(const std::int32_t instance) const
    -> const api::client::internal::Manager*
{
    if (is_server_session(instance)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Error: provided instance ")(
            instance)(" is a server session.")
            .Flush();

        return nullptr;
    } else {
        try {
            return &dynamic_cast<const api::client::internal::Manager&>(
                ot_.Client(static_cast<int>(get_index(instance))));
        } catch (...) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Error: provided instance ")(
                instance)(" is not a valid client session.")
                .Flush();

            return nullptr;
        }
    }
}

auto RPC::get_compatible_accounts(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_CLIENT_ONLY();
    CHECK_OWNER();
    CHECK_INPUT(identifier, proto::RPCRESPONSE_INVALID);

    const auto workflowID = Identifier::Factory(command.identifier(0));
    const auto pWorkflow = client.Workflow().LoadWorkflow(ownerID, workflowID);

    if (false == bool(pWorkflow)) {
        add_output_status(output, proto::RPCRESPONSE_WORKFLOW_NOT_FOUND);

        return output;
    }

    const auto& workflow = *pWorkflow;
    auto unitID = identifier::UnitDefinition::Factory();

    switch (api::client::internal::translate(workflow.type())) {
        case api::client::PaymentWorkflowType::IncomingCheque:
        case api::client::PaymentWorkflowType::IncomingInvoice: {
            auto chequeState =
                opentxs::api::client::Workflow::InstantiateCheque(
                    client, workflow);
            const auto& [state, cheque] = chequeState;

            if (false == bool(cheque)) {
                add_output_status(output, proto::RPCRESPONSE_CHEQUE_NOT_FOUND);

                return output;
            }

            unitID->Assign(cheque->GetInstrumentDefinitionID());
        } break;
        default: {
            add_output_status(output, proto::RPCRESPONSE_CHEQUE_NOT_FOUND);

            return output;
        }
    }

    const auto owneraccounts = client.Storage().AccountsByOwner(ownerID);
    const auto unitaccounts = client.Storage().AccountsByContract(unitID);
    std::vector<OTIdentifier> compatible{};
    std::set_intersection(
        owneraccounts.begin(),
        owneraccounts.end(),
        unitaccounts.begin(),
        unitaccounts.end(),
        std::back_inserter(compatible));

    for (const auto& accountid : compatible) {
        output.add_identifier(accountid->str());
    }

    if (0 == output.identifier_size()) {
        add_output_status(output, proto::RPCRESPONSE_NONE);
    } else {
        add_output_status(output, proto::RPCRESPONSE_SUCCESS);
    }

    return output;
}

auto RPC::get_index(const std::int32_t instance) -> std::size_t
{
    return (instance - (instance % 2)) / 2;
}

auto RPC::get_nyms(const proto::RPCCommand& command) const -> proto::RPCResponse
{
    INIT_SESSION();
    CHECK_INPUT(identifier, proto::RPCRESPONSE_INVALID);

    for (const auto& id : command.identifier()) {
        auto pNym = session.Wallet().Nym(identifier::Nym::Factory(id));

        if (pNym) {
            const auto& nym = *pNym;
            *output.add_nym() = nym.asPublicNym();
            add_output_status(output, proto::RPCRESPONSE_SUCCESS);
        } else {
            add_output_status(output, proto::RPCRESPONSE_NYM_NOT_FOUND);
        }
    }

    return output;
}

auto RPC::get_pending_payments(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_CLIENT_ONLY();
    CHECK_OWNER();

    const auto& workflow = client.Workflow();
    auto checkWorkflows = workflow.List(
        ownerID,
        api::client::PaymentWorkflowType::IncomingCheque,
        api::client::PaymentWorkflowState::Conveyed);
    auto invoiceWorkflows = workflow.List(
        ownerID,
        api::client::PaymentWorkflowType::IncomingInvoice,
        api::client::PaymentWorkflowState::Conveyed);
    std::set<OTIdentifier> workflows;
    std::set_union(
        checkWorkflows.begin(),
        checkWorkflows.end(),
        invoiceWorkflows.begin(),
        invoiceWorkflows.end(),
        std::inserter(workflows, workflows.end()));

    for (auto workflowID : workflows) {
        const auto paymentWorkflow = workflow.LoadWorkflow(ownerID, workflowID);

        if (false == bool(paymentWorkflow)) { continue; }

        auto chequeState = opentxs::api::client::Workflow::InstantiateCheque(
            client, *paymentWorkflow);

        const auto& [state, cheque] = chequeState;

        if (false == bool(cheque)) { continue; }

        auto& accountEvent = *output.add_accountevent();
        accountEvent.set_version(ACCOUNTEVENT_VERSION);
        auto accountEventType = proto::ACCOUNTEVENT_INCOMINGCHEQUE;

        if (api::client::PaymentWorkflowType::IncomingInvoice ==
            api::client::internal::translate(paymentWorkflow->type())) {
            accountEventType = proto::ACCOUNTEVENT_INCOMINGINVOICE;
        }

        accountEvent.set_type(accountEventType);
        const auto contactID =
            client.Contacts().ContactID(cheque->GetSenderNymID());
        accountEvent.set_contact(contactID->str());
        accountEvent.set_workflow(paymentWorkflow->id());
        accountEvent.set_pendingamount(cheque->GetAmount());

        if (0 < paymentWorkflow->event_size()) {
            const auto paymentEvent = paymentWorkflow->event(0);
            accountEvent.set_timestamp(paymentEvent.time());
        }

        accountEvent.set_memo(cheque->GetMemo().Get());
    }

    if (0 == output.accountevent_size()) {
        add_output_status(output, proto::RPCRESPONSE_NONE);
    } else {
        add_output_status(output, proto::RPCRESPONSE_SUCCESS);
    }

    return output;
}

auto RPC::get_seeds(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_SESSION();
    CHECK_INPUT(identifier, proto::RPCRESPONSE_INVALID);

#if OT_CRYPTO_WITH_BIP32
    const auto& hdseeds = session.Seeds();

    for (const auto& id : command.identifier()) {
        auto words = hdseeds.Words(id, reason);
        auto passphrase = hdseeds.Passphrase(id, reason);

        if (false == words.empty() || false == passphrase.empty()) {
            auto& seed = *output.add_seed();
            seed.set_version(SEED_VERSION);
            seed.set_id(id);
            seed.set_words(words);
            seed.set_passphrase(passphrase);
            add_output_status(output, proto::RPCRESPONSE_SUCCESS);
        } else {
            add_output_status(output, proto::RPCRESPONSE_NONE);
        }
    }
#endif  // OT_CRYPTO_WITH_BIP32

    return output;
}

auto RPC::get_server(const std::int32_t instance) const
    -> const api::server::Manager*
{
    if (is_client_session(instance)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Error: provided instance ")(
            instance)(" is a client session.")
            .Flush();

        return nullptr;
    } else {
        try {
            return &ot_.Server(static_cast<int>(get_index(instance)));
        } catch (...) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Error: provided instance ")(
                instance)(" is not a valid server session.")
                .Flush();

            return nullptr;
        }
    }
}

auto RPC::get_server_admin_nym(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_SERVER_ONLY();

    const auto password = server.GetAdminNym();

    if (password.empty()) {
        add_output_status(output, proto::RPCRESPONSE_NONE);
    } else {
        output.add_identifier(password);
        add_output_status(output, proto::RPCRESPONSE_SUCCESS);
    }

    return output;
}

auto RPC::get_server_contracts(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_SESSION();
    CHECK_INPUT(identifier, proto::RPCRESPONSE_INVALID);

    for (const auto& id : command.identifier()) {
        try {
            const auto contract =
                session.Wallet().Server(identifier::Server::Factory(id));
            auto serialized = proto::ServerContract{};
            if (false == contract->Serialize(serialized, true)) {
                add_output_status(output, proto::RPCRESPONSE_NONE);
            } else {
                *output.add_notary() = serialized;
                add_output_status(output, proto::RPCRESPONSE_SUCCESS);
            }
        } catch (...) {
            add_output_status(output, proto::RPCRESPONSE_NONE);
        }
    }

    return output;
}

auto RPC::get_server_password(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_SERVER_ONLY();

    const auto password = server.GetAdminPassword();

    if (password.empty()) {
        add_output_status(output, proto::RPCRESPONSE_NONE);
    } else {
        output.add_identifier(password);
        add_output_status(output, proto::RPCRESPONSE_SUCCESS);
    }

    return output;
}

auto RPC::get_session(const std::int32_t instance) const -> const api::Core&
{
    if (is_server_session(instance)) {
        return ot_.Server(static_cast<int>(get_index(instance)));
    } else {
        return ot_.Client(static_cast<int>(get_index(instance)));
    }
}

auto RPC::get_transaction_data(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_CLIENT_ONLY();
    CHECK_INPUT(identifier, proto::RPCRESPONSE_INVALID);

    for ([[maybe_unused]] const auto& id : command.identifier()) {
        add_output_status(output, proto::RPCRESPONSE_UNIMPLEMENTED);
    }

    return output;
}

auto RPC::get_unit_definitions(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_SESSION();
    CHECK_INPUT(identifier, proto::RPCRESPONSE_INVALID);

    for (const auto& id : command.identifier()) {
        try {
            const auto contract = session.Wallet().UnitDefinition(
                identifier::UnitDefinition::Factory(id));

            if (contract->Version() > 1 && command.version() < 3) {
                add_output_status(output, proto::RPCRESPONSE_INVALID);

            } else {
                *output.add_unit() = contract->PublicContract();
                add_output_status(output, proto::RPCRESPONSE_SUCCESS);
            }
        } catch (...) {
            add_output_status(output, proto::RPCRESPONSE_NONE);
        }
    }

    return output;
}

auto RPC::get_workflow(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_CLIENT_ONLY();
    CHECK_INPUT(getworkflow, proto::RPCRESPONSE_INVALID);

    for (const auto& getworkflow : command.getworkflow()) {
        const auto workflow = client.Workflow().LoadWorkflow(
            identifier::Nym::Factory(getworkflow.nymid()),
            Identifier::Factory(getworkflow.workflowid()));

        if (workflow) {
            auto& paymentworkflow = *output.add_workflow();
            paymentworkflow = *workflow;
            add_output_status(output, proto::RPCRESPONSE_SUCCESS);
        } else {
            add_output_status(output, proto::RPCRESPONSE_NONE);
        }
    }

    return output;
}

auto RPC::immediate_create_account(
    const api::client::internal::Manager& client,
    const identifier::Nym& owner,
    const identifier::Server& notary,
    const identifier::UnitDefinition& unit) const -> bool
{
    const auto registered =
        client.OTAPI().IsNym_RegisteredAtServer(owner, notary);

    try {
        client.Wallet().UnitDefinition(unit);
    } catch (...) {

        return false;
    }

    return registered;
}

auto RPC::immediate_register_issuer_account(
    const api::client::internal::Manager& client,
    const identifier::Nym& owner,
    const identifier::Server& notary) const -> bool
{
    return client.OTAPI().IsNym_RegisteredAtServer(owner, notary);
}

auto RPC::immediate_register_nym(
    const api::client::internal::Manager& client,
    const identifier::Server& notary) const -> bool
{
    try {
        client.Wallet().Server(notary);

        return true;
    } catch (...) {

        return false;
    }
}

auto RPC::import_seed(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_SESSION();

#if OT_CRYPTO_WITH_BIP32
    auto& seed = command.hdseed();
    auto words = ot_.Factory().SecretFromText(seed.words());
    auto passphrase = ot_.Factory().SecretFromText(seed.passphrase());
    const auto identifier = session.Seeds().ImportSeed(
        words,
        passphrase,
        crypto::SeedStyle::BIP39,
        crypto::Language::en,
        reason);

    if (identifier.empty()) {
        add_output_status(output, proto::RPCRESPONSE_INVALID);
    } else {
        output.add_identifier(identifier);
        add_output_status(output, proto::RPCRESPONSE_SUCCESS);
    }
#endif  // OT_CRYPTO_WITH_BIP32

    return output;
}

auto RPC::import_server_contract(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_SESSION();
    CHECK_INPUT(server, proto::RPCRESPONSE_INVALID);

    for (const auto& servercontract : command.server()) {
        try {
            session.Wallet().Server(servercontract);
            add_output_status(output, proto::RPCRESPONSE_SUCCESS);
        } catch (...) {
            add_output_status(output, proto::RPCRESPONSE_NONE);
        }
    }

    return output;
}

auto RPC::init(const proto::RPCCommand& command) -> proto::RPCResponse
{
    proto::RPCResponse output{};
    output.set_version(command.version());
    output.set_cookie(command.cookie());
    output.set_type(command.type());

    return output;
}

auto RPC::invalid_command(const proto::RPCCommand& command)
    -> proto::RPCResponse
{
    INIT();

    add_output_status(output, proto::RPCRESPONSE_INVALID);

    return output;
}

auto RPC::is_blockchain_account(const request::Base& base, const Identifier& id)
    const noexcept -> bool
{
    try {
        const auto& api = client_session(base);
        const auto [chain, owner] = api.Blockchain().LookupAccount(id);

        if (blockchain::Type::Unknown == chain) { return false; }

        return true;
    } catch (...) {

        return false;
    }
}

auto RPC::is_client_session(std::int32_t instance) const -> bool
{
    return instance % 2 == 0;
}

auto RPC::is_server_session(std::int32_t instance) const -> bool
{
    return instance % 2 != 0;
}

auto RPC::is_session_valid(std::int32_t instance) const -> bool
{
    auto lock = Lock{lock_};
    auto index = get_index(instance);

    if (is_server_session(instance)) {
        return ot_.Servers() > index;
    } else {
        return ot_.Clients() > index;
    }
}

auto RPC::list_client_sessions(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT();
    auto lock = Lock{lock_};

    for (std::size_t i = 0; i < ot_.Clients(); ++i) {
        proto::SessionData& data = *output.add_sessions();
        data.set_version(SESSION_DATA_VERSION);
        data.set_instance(ot_.Client(static_cast<int>(i)).Instance());
    }

    if (0 == output.sessions_size()) {
        add_output_status(output, proto::RPCRESPONSE_NONE);
    } else {
        add_output_status(output, proto::RPCRESPONSE_SUCCESS);
    }

    return output;
}

auto RPC::list_contacts(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_CLIENT_ONLY();

    auto contacts = client.Contacts().ContactList();

    for (const auto& contact : contacts) {
        output.add_identifier(std::get<0>(contact));
    }

    if (0 == output.identifier_size()) {
        add_output_status(output, proto::RPCRESPONSE_NONE);
    } else {
        add_output_status(output, proto::RPCRESPONSE_SUCCESS);
    }

    return output;
}

auto RPC::list_seeds(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_SESSION();

    const auto& seeds = session.Storage().SeedList();

    if (0 < seeds.size()) {
        for (const auto& seed : seeds) { output.add_identifier(seed.first); }
    }

    if (0 == output.identifier_size()) {
        add_output_status(output, proto::RPCRESPONSE_NONE);
    } else {
        add_output_status(output, proto::RPCRESPONSE_SUCCESS);
    }

    return output;
}

auto RPC::list_server_contracts(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_SESSION();

    const auto& servers = session.Wallet().ServerList();

    for (const auto& server : servers) { output.add_identifier(server.first); }

    if (0 == output.identifier_size()) {
        add_output_status(output, proto::RPCRESPONSE_NONE);
    } else {
        add_output_status(output, proto::RPCRESPONSE_SUCCESS);
    }

    return output;
}

auto RPC::list_server_sessions(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT();
    auto lock = Lock{lock_};

    for (std::size_t i = 0; i < ot_.Servers(); ++i) {
        auto& data = *output.add_sessions();
        data.set_version(SESSION_DATA_VERSION);
        data.set_instance(ot_.Server(static_cast<int>(i)).Instance());
    }

    if (0 == output.sessions_size()) {
        add_output_status(output, proto::RPCRESPONSE_NONE);
    } else {
        add_output_status(output, proto::RPCRESPONSE_SUCCESS);
    }

    return output;
}

auto RPC::list_unit_definitions(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_SESSION();

    const auto& unitdefinitions = session.Wallet().UnitDefinitionList();

    for (const auto& unitdefinition : unitdefinitions) {
        output.add_identifier(unitdefinition.first);
    }

    if (0 == output.identifier_size()) {
        add_output_status(output, proto::RPCRESPONSE_NONE);
    } else {
        add_output_status(output, proto::RPCRESPONSE_SUCCESS);
    }

    return output;
}

auto RPC::lookup_account_id(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_SESSION();

    const auto& label = command.param();

    for (const auto& [id, alias] : session.Storage().AccountList()) {
        if (alias == label) { output.add_identifier(id); }
    }

    if (0 == output.identifier_size()) {
        add_output_status(output, proto::RPCRESPONSE_NONE);
    } else {
        add_output_status(output, proto::RPCRESPONSE_SUCCESS);
    }

    return output;
}

auto RPC::move_funds(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_CLIENT_ONLY();

    const auto& movefunds = command.movefunds();
    const auto sourceaccount = Identifier::Factory(movefunds.sourceaccount());
    auto sender = client.Storage().AccountOwner(sourceaccount);

    switch (movefunds.type()) {
        case proto::RPCPAYMENTTYPE_TRANSFER: {
            const auto targetaccount =
                Identifier::Factory(movefunds.destinationaccount());
            const auto notary = client.Storage().AccountServer(sourceaccount);

            INIT_OTX(
                SendTransfer,
                sender,
                notary,
                sourceaccount,
                targetaccount,
                movefunds.amount(),
                movefunds.memo());

            if (false == ready) {
                add_output_status(output, proto::RPCRESPONSE_ERROR);

                return output;
            }

            evaluate_move_funds(client, future.get(), output);
        } break;
        case proto::RPCPAYMENTTYPE_CHEQUE:
            [[fallthrough]];
        case proto::RPCPAYMENTTYPE_VOUCHER:
            [[fallthrough]];
        case proto::RPCPAYMENTTYPE_INVOICE:
            [[fallthrough]];
        case proto::RPCPAYMENTTYPE_BLINDED:
            [[fallthrough]];
        default: {
            add_output_status(output, proto::RPCRESPONSE_INVALID);
        }
    }

    return output;
}

auto RPC::Process(const proto::RPCCommand& command) const -> proto::RPCResponse
{
    const auto valid = proto::Validate(command, VERBOSE);

    if (false == valid) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid serialized command")
            .Flush();

        return invalid_command(command);
    }

    switch (command.type()) {
        case proto::RPCCOMMAND_LISTNYMS:
        case proto::RPCCOMMAND_LISTACCOUNTS:
        case proto::RPCCOMMAND_GETACCOUNTBALANCE:
        case proto::RPCCOMMAND_GETACCOUNTACTIVITY:
        case proto::RPCCOMMAND_SENDPAYMENT: {
            const auto response = Process(*request::Factory(command));
            auto output = proto::RPCResponse{};
            response->Serialize(output);

            return output;
        }
        case proto::RPCCOMMAND_ADDCLIENTSESSION: {
            return start_client(command);
        }
        case proto::RPCCOMMAND_ADDSERVERSESSION: {
            return start_server(command);
        }
        case proto::RPCCOMMAND_LISTCLIENTSESSIONS: {
            return list_client_sessions(command);
        }
        case proto::RPCCOMMAND_LISTSERVERSESSIONS: {
            return list_server_sessions(command);
        }
        case proto::RPCCOMMAND_IMPORTHDSEED: {
            return import_seed(command);
        }
        case proto::RPCCOMMAND_LISTHDSEEDS: {
            return list_seeds(command);
        }
        case proto::RPCCOMMAND_GETHDSEED: {
            return get_seeds(command);
        }
        case proto::RPCCOMMAND_CREATENYM: {
            return create_nym(command);
        }
        case proto::RPCCOMMAND_GETNYM: {
            return get_nyms(command);
        }
        case proto::RPCCOMMAND_ADDCLAIM: {
            return add_claim(command);
        }
        case proto::RPCCOMMAND_DELETECLAIM: {
            return delete_claim(command);
        }
        case proto::RPCCOMMAND_IMPORTSERVERCONTRACT: {
            return import_server_contract(command);
        }
        case proto::RPCCOMMAND_LISTSERVERCONTRACTS: {
            return list_server_contracts(command);
        }
        case proto::RPCCOMMAND_REGISTERNYM: {
            return register_nym(command);
        }
        case proto::RPCCOMMAND_CREATEUNITDEFINITION: {
            return create_unit_definition(command);
        }
        case proto::RPCCOMMAND_LISTUNITDEFINITIONS: {
            return list_unit_definitions(command);
        }
        case proto::RPCCOMMAND_ISSUEUNITDEFINITION: {
            return create_issuer_account(command);
        }
        case proto::RPCCOMMAND_CREATEACCOUNT: {
            return create_account(command);
        }
        case proto::RPCCOMMAND_MOVEFUNDS: {
            return move_funds(command);
        }
        case proto::RPCCOMMAND_GETSERVERCONTRACT: {
            return get_server_contracts(command);
        }
        case proto::RPCCOMMAND_ADDCONTACT: {
            return add_contact(command);
        }
        case proto::RPCCOMMAND_LISTCONTACTS: {
            return list_contacts(command);
        }
        case proto::RPCCOMMAND_GETCONTACT:
        case proto::RPCCOMMAND_ADDCONTACTCLAIM:
        case proto::RPCCOMMAND_DELETECONTACTCLAIM:
        case proto::RPCCOMMAND_VERIFYCLAIM:
        case proto::RPCCOMMAND_ACCEPTVERIFICATION:
        case proto::RPCCOMMAND_SENDCONTACTMESSAGE:
        case proto::RPCCOMMAND_GETCONTACTACTIVITY: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Command not implemented.")
                .Flush();
        } break;
        case proto::RPCCOMMAND_ACCEPTPENDINGPAYMENTS: {
            return accept_pending_payments(command);
        }
        case proto::RPCCOMMAND_GETPENDINGPAYMENTS: {
            return get_pending_payments(command);
        }
        case proto::RPCCOMMAND_GETCOMPATIBLEACCOUNTS: {
            return get_compatible_accounts(command);
        }
        case proto::RPCCOMMAND_CREATECOMPATIBLEACCOUNT: {
            return create_compatible_account(command);
        }
        case proto::RPCCOMMAND_GETWORKFLOW: {
            return get_workflow(command);
        }
        case proto::RPCCOMMAND_GETSERVERPASSWORD: {
            return get_server_password(command);
        }
        case proto::RPCCOMMAND_GETADMINNYM: {
            return get_server_admin_nym(command);
        }
        case proto::RPCCOMMAND_GETUNITDEFINITION: {
            return get_unit_definitions(command);
        }
        case proto::RPCCOMMAND_GETTRANSACTIONDATA: {
            return get_transaction_data(command);
        }
        case proto::RPCCOMMAND_LOOKUPACCOUNTID: {
            return lookup_account_id(command);
        }
        case proto::RPCCOMMAND_RENAMEACCOUNT: {
            return rename_account(command);
        }
        case proto::RPCCOMMAND_ERROR:
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported command.")
                .Flush();
        }
    }

    return invalid_command(command);
}

auto RPC::Process(const request::Base& command) const
    -> std::unique_ptr<response::Base>
{
    switch (command.Type()) {
        case CommandType::list_nyms: {
            return list_nyms(command);
        }
        case CommandType::list_accounts: {
            return list_accounts(command);
        }
        case CommandType::get_account_balance: {
            return get_account_balance(command);
        }
        case CommandType::get_account_activity: {
            return get_account_activity(command);
        }
        case CommandType::send_payment: {
            return send_payment(command);
        }
        case CommandType::add_client_session:
        case CommandType::add_server_session:
        case CommandType::list_client_sessions:
        case CommandType::list_server_sessions:
        case CommandType::import_hd_seed:
        case CommandType::list_hd_seeds:
        case CommandType::get_hd_seed:
        case CommandType::create_nym:
        case CommandType::get_nym:
        case CommandType::add_claim:
        case CommandType::delete_claim:
        case CommandType::import_server_contract:
        case CommandType::list_server_contracts:
        case CommandType::register_nym:
        case CommandType::create_unit_definition:
        case CommandType::list_unit_definitions:
        case CommandType::issue_unit_definition:
        case CommandType::create_account:
        case CommandType::move_funds:
        case CommandType::add_contact:
        case CommandType::list_contacts:
        case CommandType::get_contact:
        case CommandType::add_contact_claim:
        case CommandType::delete_contact_claim:
        case CommandType::verify_claim:
        case CommandType::accept_verification:
        case CommandType::send_contact_message:
        case CommandType::get_contact_activity:
        case CommandType::get_server_contract:
        case CommandType::get_pending_payments:
        case CommandType::accept_pending_payments:
        case CommandType::get_compatible_accounts:
        case CommandType::create_compatible_account:
        case CommandType::get_workflow:
        case CommandType::get_server_password:
        case CommandType::get_admin_nym:
        case CommandType::get_unit_definition:
        case CommandType::get_transaction_data:
        case CommandType::lookup_accountid:
        case CommandType::rename_account:
        case CommandType::error:
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported command.")
                .Flush();

            return std::make_unique<response::Invalid>(command);
        }
    }
}

auto RPC::queue_task(
    const identifier::Nym& nymID,
    const std::string taskID,
    Finish&& finish,
    Future&& future,
    proto::RPCResponse& output) const -> void
{
    auto lock = Lock{task_lock_};
    queued_tasks_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(taskID),
        std::forward_as_tuple(std::move(future), std::move(finish), nymID));
    auto taskIDCompat = Identifier::Factory();
    taskIDCompat->CalculateDigest(taskID);
    add_output_task(output, taskIDCompat->str());
    add_output_status(output, proto::RPCRESPONSE_QUEUED);
}

auto RPC::queue_task(
    const api::Core& api,
    const identifier::Nym& nymID,
    const std::string taskID,
    Finish&& finish,
    Future&& future) const noexcept -> std::string
{
    {
        auto lock = Lock{task_lock_};
        queued_tasks_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(taskID),
            std::forward_as_tuple(std::move(future), std::move(finish), nymID));
    }

    const auto hashedID = [&] {
        auto out = api.Factory().Identifier();
        out->CalculateDigest(taskID);

        return out;
    }();

    return hashedID->str();
}

auto RPC::register_nym(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_CLIENT_ONLY();
    CHECK_OWNER();

    const auto notaryID = identifier::Server::Factory(command.notary());
    auto registered =
        client.OTAPI().IsNym_RegisteredAtServer(ownerID, notaryID);

    if (registered) {
        add_output_status(output, proto::RPCRESPONSE_UNNECESSARY);

        return output;
    }

    INIT_OTX(RegisterNymPublic, ownerID, notaryID, true);

    if (false == ready) {
        add_output_status(output, proto::RPCRESPONSE_REGISTER_NYM_FAILED);

        return output;
    }

    if (immediate_register_nym(client, notaryID)) {
        evaluate_register_nym(future.get(), output);
    } else {
        queue_task(
            ownerID,
            std::to_string(taskID),
            [&](const auto& in, auto& out) -> void {
                evaluate_register_nym(in, out);
            },
            std::move(future),
            output);
    }

    return output;
}

auto RPC::rename_account(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT_CLIENT_ONLY();
    CHECK_INPUT(modifyaccount, proto::RPCRESPONSE_INVALID);

    for (const auto& rename : command.modifyaccount()) {
        const auto accountID = Identifier::Factory(rename.accountid());
        auto account = client.Wallet().mutable_Account(accountID, reason);

        if (account) {
            account.get().SetAlias(rename.label());
            add_output_status(output, proto::RPCRESPONSE_SUCCESS);
        } else {
            add_output_status(output, proto::RPCRESPONSE_ACCOUNT_NOT_FOUND);
        }
    }

    return output;
}

auto RPC::session(const request::Base& command) const noexcept(false)
    -> const api::Core&
{
    const auto session = command.Session();

    if (false == is_session_valid(session)) {

        throw std::runtime_error{"invalid session"};
    }

    if (is_client_session(session)) {

        return ot_.Client(static_cast<int>(get_index(session)));
    } else {

        return ot_.Server(static_cast<int>(get_index(session)));
    }
}

auto RPC::start_client(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT();
    auto lock = Lock{lock_};
    const auto session{static_cast<std::uint32_t>(ot_.Clients())};

    std::uint32_t instance{0};

    try {
        auto& manager = ot_.StartClient(get_args(command.arg()), session);
        instance = manager.Instance();

        auto bound =
            task_subscriber_->Start(manager.Endpoints().TaskComplete());

        OT_ASSERT(bound)

    } catch (...) {
        add_output_status(output, proto::RPCRESPONSE_INVALID);

        return output;
    }

    output.set_session(instance);
    add_output_status(output, proto::RPCRESPONSE_SUCCESS);

    return output;
}

auto RPC::start_server(const proto::RPCCommand& command) const
    -> proto::RPCResponse
{
    INIT();
    auto lock = Lock{lock_};
    const auto session{static_cast<std::uint32_t>(ot_.Servers())};

    std::uint32_t instance{0};

    try {
        auto& manager = ot_.StartServer(get_args(command.arg()), session);
        instance = manager.Instance();
    } catch (const std::invalid_argument&) {
        add_output_status(output, proto::RPCRESPONSE_BAD_SERVER_ARGUMENT);
        return output;
    } catch (...) {
        add_output_status(output, proto::RPCRESPONSE_INVALID);
        return output;
    }

    output.set_session(instance);
    add_output_status(output, proto::RPCRESPONSE_SUCCESS);

    return output;
}

auto RPC::status(const response::Base::Identifiers& ids) const noexcept
    -> ResponseCode
{
    return (0u == ids.size()) ? ResponseCode::none : ResponseCode::success;
}

void RPC::task_handler(const zmq::Message& in)
{
    const auto body = in.Body();

    OT_ASSERT(2 < body.size());

    using ID = api::client::OTX::TaskID;

    const auto taskID = std::to_string(body.at(1).as<ID>());
    const auto success = body.at(2).as<bool>();
    LogTrace(OT_METHOD)(__FUNCTION__)(": Received notice for task ")(taskID)
        .Flush();
    auto lock = Lock{task_lock_};
    auto it = queued_tasks_.find(taskID);
    lock.unlock();
    proto::RPCPush message{};
    auto& task = *message.mutable_taskcomplete();

    if (queued_tasks_.end() != it) {
        auto& [future, finish, nymID] = it->second;
        message.set_id(nymID->str());

        if (finish) { finish(future.get(), task); }
    } else {
        LogTrace(OT_METHOD)(__FUNCTION__)(": We don't care about task ")(taskID)
            .Flush();

        return;
    }

    lock.lock();
    queued_tasks_.erase(it);
    lock.unlock();
    message.set_version(RPCPUSH_VERSION);
    message.set_type(proto::RPCPUSH_TASK);
    task.set_version(TASKCOMPLETE_VERSION);
    const auto taskIDStr = String::Factory(taskID);
    auto taskIDCompat = Identifier::Factory();
    taskIDCompat->CalculateDigest(taskIDStr->Bytes());
    task.set_id(taskIDCompat->str());
    task.set_result(success);
    auto output = zmq::Message::Factory();
    output->AddFrame(message);
    rpc_publisher_->Send(output);
}

RPC::~RPC()
{
    task_subscriber_->Close();
    rpc_publisher_->Close();
    push_receiver_->Close();
}
}  // namespace opentxs::rpc::implementation
