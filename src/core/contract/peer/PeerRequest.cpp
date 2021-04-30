// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                        // IWYU pragma: associated
#include "1_Internal.hpp"                      // IWYU pragma: associated
#include "core/contract/peer/PeerRequest.hpp"  // IWYU pragma: associated

#include <list>
#include <memory>

#include "internal/api/Api.hpp"
#include "internal/core/contract/Contract.hpp"
#include "internal/core/contract/peer/Factory.hpp"
#include "internal/core/contract/peer/Peer.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/contract/peer/PeerRequest.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/crypto/SignatureRole.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/PeerRequest.pb.h"
#include "opentxs/protobuf/Signature.pb.h"
#include "opentxs/protobuf/verify/PeerRequest.hpp"

#define OT_METHOD "opentxs::contract::peer::implementation::Request::"

namespace opentxs::factory
{
auto PeerRequest(const api::Core& api) noexcept
    -> std::unique_ptr<contract::peer::Request>
{
    return std::make_unique<contract::peer::blank::Request>(api);
}
}  // namespace opentxs::factory

namespace opentxs::contract::peer::implementation
{
Request::Request(
    const api::internal::Core& api,
    const Nym_p& nym,
    const VersionNumber version,
    const identifier::Nym& recipient,
    const identifier::Server& server,
    const PeerRequestType& type,
    const std::string& conditions)
    : Signable(api, nym, version, conditions, "")
    , initiator_(nym->ID())
    , recipient_(recipient)
    , server_(server)
    , cookie_(Identifier::Random())
    , type_(type)
{
}

Request::Request(
    const api::internal::Core& api,
    const Nym_p& nym,
    const SerializedType& serialized,
    const std::string& conditions)
    : Signable(
          api,
          nym,
          serialized.version(),
          conditions,
          "",
          api.Factory().Identifier(serialized.id()),
          serialized.has_signature()
              ? Signatures{std::make_shared<proto::Signature>(
                    serialized.signature())}
              : Signatures{})
    , initiator_(api.Factory().NymID(serialized.initiator()))
    , recipient_(api.Factory().NymID(serialized.recipient()))
    , server_(api.Factory().ServerID(serialized.server()))
    , cookie_(Identifier::Factory(serialized.cookie()))
    , type_(internal::translate(serialized.type()))
{
}

Request::Request(const Request& rhs) noexcept
    : Signable(rhs)
    , initiator_(rhs.initiator_)
    , recipient_(rhs.recipient_)
    , server_(rhs.server_)
    , cookie_(rhs.cookie_)
    , type_(rhs.type_)
{
}

auto Request::contract(const Lock& lock) const -> SerializedType
{
    auto contract = SigVersion(lock);
    if (0 < signatures_.size()) {
        *(contract.mutable_signature()) = *(signatures_.front());
    }

    return contract;
}

auto Request::Contract() const -> SerializedType
{
    Lock lock(lock_);

    return contract(lock);
}

auto Request::FinalizeContract(Request& contract, const PasswordPrompt& reason)
    -> bool
{
    Lock lock(contract.lock_);

    if (!contract.update_signature(lock, reason)) { return false; }

    return contract.validate(lock);
}

auto Request::Finish(Request& contract, const PasswordPrompt& reason) -> bool
{
    if (FinalizeContract(contract, reason)) {

        return true;
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to finalize contract.")
            .Flush();

        return false;
    }
}

auto Request::GetID(const Lock& lock) const -> OTIdentifier
{
    return GetID(api_, IDVersion(lock));
}

auto Request::GetID(
    const api::internal::Core& api,
    const SerializedType& contract) -> OTIdentifier
{
    return api.Factory().Identifier(contract);
}

auto Request::IDVersion(const Lock& lock) const -> SerializedType
{
    OT_ASSERT(verify_write_lock(lock));

    SerializedType contract;

    if (version_ < 2) {
        contract.set_version(2);
    } else {
        contract.set_version(version_);
    }

    contract.clear_id();  // reinforcing that this field must be blank.
    contract.set_initiator(String::Factory(initiator_)->Get());
    contract.set_recipient(String::Factory(recipient_)->Get());
    contract.set_type(internal::translate(type_));
    contract.set_cookie(String::Factory(cookie_)->Get());
    contract.set_server(String::Factory(server_)->Get());
    contract.clear_signature();  // reinforcing that this field must be blank.

    return contract;
}

auto Request::Serialize() const -> OTData
{
    Lock lock(lock_);

    return api_.Factory().Data(contract(lock));
}

auto Request::SigVersion(const Lock& lock) const -> SerializedType
{
    auto contract = IDVersion(lock);
    contract.set_id(String::Factory(id(lock))->Get());

    return contract;
}

auto Request::update_signature(const Lock& lock, const PasswordPrompt& reason)
    -> bool
{
    if (!Signable::update_signature(lock, reason)) { return false; }

    bool success = false;
    signatures_.clear();
    auto serialized = SigVersion(lock);
    auto& signature = *serialized.mutable_signature();
    success = nym_->Sign(
        serialized, crypto::SignatureRole::PeerRequest, signature, reason);

    if (success) {
        signatures_.emplace_front(new proto::Signature(signature));
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to create signature.")
            .Flush();
    }

    return success;
}

auto Request::validate(const Lock& lock) const -> bool
{
    bool validNym = false;

    if (nym_) {
        validNym = nym_->VerifyPseudonym();
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid nym.").Flush();
    }

    const bool validSyntax = proto::Validate(contract(lock), VERBOSE);

    if (!validSyntax) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid syntax.").Flush();
    }

    if (1 > signatures_.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing signature.").Flush();

        return false;
    }

    bool validSig = false;
    auto& signature = *signatures_.cbegin();

    if (signature) { validSig = verify_signature(lock, *signature); }

    if (!validSig) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid signature.").Flush();
    }

    return (validNym && validSyntax && validSig);
}

auto Request::verify_signature(
    const Lock& lock,
    const proto::Signature& signature) const -> bool
{
    if (!Signable::verify_signature(lock, signature)) { return false; }

    auto serialized = SigVersion(lock);
    auto& sigProto = *serialized.mutable_signature();
    sigProto.CopyFrom(signature);

    return nym_->Verify(serialized, sigProto);
}
}  // namespace opentxs::contract::peer::implementation
