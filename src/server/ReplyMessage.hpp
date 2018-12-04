// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

#include "opentxs/api/Editor.hpp"
#include "opentxs/core/Nym.hpp"
#include "opentxs/Types.hpp"

#include <cstdint>
#include <memory>
#include <set>

namespace opentxs::server
{
class ReplyMessage
{
public:
    ReplyMessage(
        const UserCommandProcessor& parent,
        const opentxs::api::Wallet& wallet,
        const Identifier& notaryID,
        const Nym& signer,
        const Message& input,
        Server& server,
        const MessageType& type,
        Message& output);

    std::set<RequestNumber> Acknowledged() const;
    bool HaveContext() const;
    const bool& Init() const;
    const Message& Original() const;
    const bool& Success() const;

    ClientContext& Context();
    void ClearRequest();
    void DropToNymbox(const bool success);
    bool LoadContext();
    void OverrideType(const String& accountID);
    void SetAccount(const String& accountID);
    void SetAcknowledgments(const ClientContext& context);
    void SetBool(const bool value);
    void SetDepth(const std::int64_t depth);
    void SetEnum(const std::uint8_t value);
    void SetInboxHash(const Identifier& hash);
    void SetInstrumentDefinitionID(const String& id);
    void SetNymboxHash(const Identifier& hash);
    void SetOutboxHash(const Identifier& hash);
    bool SetPayload(const String& payload);
    bool SetPayload(const Data& payload);
    void SetPayload(const Armored& payload);
    bool SetPayload2(const String& payload);
    bool SetPayload3(const String& payload);
    void SetRequestNumber(const RequestNumber number);
    void SetSuccess(const bool success);
    void SetTargetNym(const String& nymID);
    void SetTransactionNumber(const TransactionNumber& number);

    ~ReplyMessage();

private:
    const UserCommandProcessor& parent_;
    const opentxs::api::Wallet& wallet_;
    const Nym& signer_;
    const Message& original_;
    const OTIdentifier notary_id_;
    Message& message_;
    Server& server_;
    bool init_{false};
    bool drop_{false};
    bool drop_status_{false};
    ConstNym sender_nym_{nullptr};
    std::unique_ptr<Editor<ClientContext>> context_{nullptr};

    void attach_request();
    void clear_request();
    bool init();
    bool init_nym();

    ReplyMessage() = delete;
    ReplyMessage(const ReplyMessage&) = delete;
    ReplyMessage(ReplyMessage&&) = delete;
    ReplyMessage& operator=(const ReplyMessage&) = delete;
    ReplyMessage& operator=(ReplyMessage&&) = delete;
};
}  // namespace opentxs::server
