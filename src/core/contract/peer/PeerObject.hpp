// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace opentxs::peer::implementation
{
class Object : virtual public opentxs::PeerObject
{
public:
    const std::unique_ptr<std::string>& Message() const override
    {
        return message_;
    }
    const ConstNym& Nym() const override { return nym_; }
    const std::unique_ptr<std::string>& Payment() const override
    {
        return payment_;
    }
#if OT_CASH
    std::shared_ptr<blind::Purse> Purse() const override { return purse_; }
#endif
    const std::shared_ptr<const PeerRequest> Request() const override
    {
        return request_;
    }
    const std::shared_ptr<const PeerReply> Reply() const override
    {
        return reply_;
    }
    proto::PeerObject Serialize() const override;
    proto::PeerObjectType Type() const override { return type_; }
    bool Validate() const override;

    std::unique_ptr<std::string>& Message() override { return message_; }
    std::unique_ptr<std::string>& Payment() override { return payment_; }
#if OT_CASH
    std::shared_ptr<blind::Purse>& Purse() override { return purse_; }
#endif

    ~Object() override = default;

private:
    friend opentxs::Factory;

    const api::Core& api_;
    ConstNym nym_{nullptr};
    std::unique_ptr<std::string> message_{nullptr};
    std::unique_ptr<std::string> payment_{nullptr};
    std::shared_ptr<const PeerReply> reply_{nullptr};
    std::shared_ptr<const PeerRequest> request_{nullptr};
#if OT_CASH
    std::shared_ptr<blind::Purse> purse_{nullptr};
#endif
    proto::PeerObjectType type_{proto::PEEROBJECT_ERROR};
    std::uint32_t version_{0};

    Object(
        const api::client::Contacts& contacts,
        const api::Core& api,
        const ConstNym& signerNym,
        const proto::PeerObject serialized);
    Object(
        const api::Core& api,
        const ConstNym& senderNym,
        const std::string& message);
#if OT_CASH
    Object(
        const api::Core& api,
        const ConstNym& senderNym,
        const std::shared_ptr<blind::Purse> purse);
#endif
    Object(
        const api::Core& api,
        const std::string& payment,
        const ConstNym& senderNym);
    Object(
        const api::Core& api,
        const std::shared_ptr<const PeerRequest> request,
        const std::shared_ptr<const PeerReply> reply,
        const std::uint32_t& version);
    Object(
        const api::Core& api,
        const std::shared_ptr<const PeerRequest> request,
        const std::uint32_t& version);
    Object(
        const api::Core& api,
        const ConstNym& nym,
        const std::string& message,
        const std::string& payment,
        const std::shared_ptr<const PeerReply> reply,
        const std::shared_ptr<const PeerRequest> request,
#if OT_CASH
        const std::shared_ptr<blind::Purse> purse,
#endif
        const proto::PeerObjectType type,
        const std::uint32_t& version);
    Object() = delete;
};
}  // namespace opentxs::peer::implementation
