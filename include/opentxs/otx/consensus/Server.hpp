// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_OTX_CONSENSUS_SERVER_HPP
#define OPENTXS_OTX_CONSENSUS_SERVER_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <future>
#include <tuple>

#include "opentxs/otx/Types.hpp"
#include "opentxs/otx/consensus/Base.hpp"
#include "opentxs/otx/consensus/ManagedNumber.hpp"

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

namespace network
{
class ServerConnection;
}  // namespace network

namespace otx
{
namespace context
{
class TransactionStatement;
}  // namespace context

class Reply;
}  // namespace otx

namespace server
{
class Server;
}  // namespace server

class Ledger;
class OTTransaction;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs
{
namespace otx
{
namespace context
{
class Server : virtual public Base
{
public:
    using DeliveryResult =
        std::pair<otx::LastReplyStatus, std::shared_ptr<Message>>;
    using SendFuture = std::future<DeliveryResult>;
    using QueueResult = std::unique_ptr<SendFuture>;
    // account label, resync nym
    using ExtraArgs = std::pair<std::string, bool>;

    OPENTXS_EXPORT virtual std::vector<OTIdentifier> Accounts() const = 0;
    OPENTXS_EXPORT virtual const std::string& AdminPassword() const = 0;
    OPENTXS_EXPORT virtual bool AdminAttempted() const = 0;
    OPENTXS_EXPORT virtual bool FinalizeServerCommand(
        Message& command,
        const PasswordPrompt& reason) const = 0;
    OPENTXS_EXPORT virtual bool HaveAdminPassword() const = 0;
    OPENTXS_EXPORT virtual bool HaveSufficientNumbers(
        const MessageType reason) const = 0;
    OPENTXS_EXPORT virtual TransactionNumber Highest() const = 0;
    OPENTXS_EXPORT virtual bool isAdmin() const = 0;
    OPENTXS_EXPORT virtual void Join() const = 0;
#if OT_CASH
    OPENTXS_EXPORT virtual std::shared_ptr<const blind::Purse> Purse(
        const identifier::UnitDefinition& id) const = 0;
#endif
    OPENTXS_EXPORT virtual std::uint64_t Revision() const = 0;
    OPENTXS_EXPORT virtual bool ShouldRename(
        const std::string& defaultName = "localhost") const = 0;
    OPENTXS_EXPORT virtual bool StaleNym() const = 0;
    OPENTXS_EXPORT virtual std::unique_ptr<Item> Statement(
        const OTTransaction& owner,
        const PasswordPrompt& reason) const = 0;
    OPENTXS_EXPORT virtual std::unique_ptr<Item> Statement(
        const OTTransaction& owner,
        const TransactionNumbers& adding,
        const PasswordPrompt& reason) const = 0;
    OPENTXS_EXPORT virtual std::unique_ptr<TransactionStatement> Statement(
        const TransactionNumbers& adding,
        const TransactionNumbers& without,
        const PasswordPrompt& reason) const = 0;
    OPENTXS_EXPORT virtual bool Verify(
        const TransactionStatement& statement) const = 0;
    OPENTXS_EXPORT virtual bool VerifyTentativeNumber(
        const TransactionNumber& number) const = 0;

    OPENTXS_EXPORT virtual bool AcceptIssuedNumber(
        const TransactionNumber& number) = 0;
    OPENTXS_EXPORT virtual bool AcceptIssuedNumbers(
        const TransactionStatement& statement) = 0;
    OPENTXS_EXPORT virtual bool AddTentativeNumber(
        const TransactionNumber& number) = 0;
    OPENTXS_EXPORT virtual network::ServerConnection& Connection() = 0;
    OPENTXS_EXPORT virtual std::pair<RequestNumber, std::unique_ptr<Message>>
    InitializeServerCommand(
        const MessageType type,
        const Armored& payload,
        const Identifier& accountID,
        const RequestNumber provided,
        const bool withAcknowledgments = true,
        const bool withNymboxHash = true) = 0;
    OPENTXS_EXPORT virtual std::pair<RequestNumber, std::unique_ptr<Message>>
    InitializeServerCommand(
        const MessageType type,
        const identifier::Nym& recipientNymID,
        const RequestNumber provided,
        const bool withAcknowledgments = true,
        const bool withNymboxHash = false) = 0;
    OPENTXS_EXPORT virtual std::pair<RequestNumber, std::unique_ptr<Message>>
    InitializeServerCommand(
        const MessageType type,
        const RequestNumber provided,
        const bool withAcknowledgments = true,
        const bool withNymboxHash = false) = 0;
#if OT_CASH
    OPENTXS_EXPORT virtual Editor<blind::Purse> mutable_Purse(
        const identifier::UnitDefinition& id,
        const PasswordPrompt& reason) = 0;
#endif
    OPENTXS_EXPORT virtual OTManagedNumber NextTransactionNumber(
        const MessageType reason) = 0;
    OPENTXS_EXPORT virtual NetworkReplyMessage PingNotary(
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual bool ProcessNotification(
        const api::client::internal::Manager& client,
        const otx::Reply& notification,
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual QueueResult Queue(
        const api::client::internal::Manager& client,
        std::shared_ptr<Message> message,
        const PasswordPrompt& reason,
        const ExtraArgs& args = ExtraArgs{}) = 0;
    OPENTXS_EXPORT virtual QueueResult Queue(
        const api::client::internal::Manager& client,
        std::shared_ptr<Message> message,
        std::shared_ptr<Ledger> inbox,
        std::shared_ptr<Ledger> outbox,
        std::set<OTManagedNumber>* numbers,
        const PasswordPrompt& reason,
        const ExtraArgs& args = ExtraArgs{}) = 0;
    OPENTXS_EXPORT virtual QueueResult RefreshNymbox(
        const api::client::internal::Manager& client,
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual bool RemoveTentativeNumber(
        const TransactionNumber& number) = 0;
    OPENTXS_EXPORT virtual void ResetThread() = 0;
    OPENTXS_EXPORT virtual bool Resync(const proto::Context& serialized) = 0;
    [[deprecated]] OPENTXS_EXPORT virtual NetworkReplyMessage SendMessage(
        const api::client::internal::Manager& client,
        const std::set<OTManagedNumber>& pending,
        Server& context,
        const Message& message,
        const PasswordPrompt& reason,
        const std::string& label = "",
        const bool resync = false) = 0;
    OPENTXS_EXPORT virtual void SetAdminAttempted() = 0;
    OPENTXS_EXPORT virtual void SetAdminPassword(
        const std::string& password) = 0;
    OPENTXS_EXPORT virtual void SetAdminSuccess() = 0;
    OPENTXS_EXPORT virtual bool SetHighest(
        const TransactionNumber& highest) = 0;
    OPENTXS_EXPORT virtual void SetPush(const bool enabled) = 0;
    OPENTXS_EXPORT virtual void SetRevision(const std::uint64_t revision) = 0;
    OPENTXS_EXPORT virtual TransactionNumber UpdateHighest(
        const TransactionNumbers& numbers,
        TransactionNumbers& good,
        TransactionNumbers& bad) = 0;
    OPENTXS_EXPORT virtual RequestNumber UpdateRequestNumber(
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual RequestNumber UpdateRequestNumber(
        bool& sendStatus,
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual bool UpdateRequestNumber(
        Message& command,
        const PasswordPrompt& reason) = 0;

    OPENTXS_EXPORT ~Server() override = default;

protected:
    Server() = default;

private:
    Server(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(const Server&) = delete;
    Server& operator=(Server&&) = delete;
};
}  // namespace context
}  // namespace otx
}  // namespace opentxs
#endif
