// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"     // IWYU pragma: associated
#include "1_Internal.hpp"   // IWYU pragma: associated
#include "otx/Request.hpp"  // IWYU pragma: associated

#include <list>
#include <memory>

#include "core/contract/Signable.hpp"
#include "internal/api/Api.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/crypto/SignatureRole.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/otx/Request.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/Nym.pb.h"
#include "opentxs/protobuf/OTXEnums.pb.h"
#include "opentxs/protobuf/ServerRequest.pb.h"
#include "opentxs/protobuf/Signature.pb.h"
#include "opentxs/protobuf/verify/ServerRequest.hpp"

template class opentxs::Pimpl<opentxs::otx::Request>;

#define OT_METHOD "opentxs::otx::implementation::Request::"

namespace opentxs::otx
{
const VersionNumber Request::DefaultVersion{2};
const VersionNumber Request::MaxVersion{2};

auto Request::Factory(
    const api::internal::Core& api,
    const Nym_p signer,
    const identifier::Server& server,
    const proto::ServerRequestType type,
    const RequestNumber number,
    const PasswordPrompt& reason) -> OTXRequest
{
    OT_ASSERT(signer);

    std::unique_ptr<implementation::Request> output{new implementation::Request(
        api, signer, signer->ID(), server, type, number)};

    OT_ASSERT(output);

    Lock lock(output->lock_);
    output->update_signature(lock, reason);

    OT_ASSERT(false == output->id(lock)->empty());

    return OTXRequest{output.release()};
}

auto Request::Factory(
    const api::internal::Core& api,
    const proto::ServerRequest serialized) -> OTXRequest
{
    return OTXRequest{new implementation::Request(api, serialized)};
}
}  // namespace opentxs::otx

namespace opentxs::otx::implementation
{
Request::Request(
    const api::internal::Core& api,
    const Nym_p signer,
    const identifier::Nym& initiator,
    const identifier::Server& server,
    const proto::ServerRequestType type,
    const RequestNumber number)
    : Signable(api, signer, DefaultVersion, "", "")
    , initiator_(initiator)
    , server_(server)
    , type_(type)
    , number_(number)
    , include_nym_(Flag::Factory(false))
{
    Lock lock(lock_);
    first_time_init(lock);
}

Request::Request(
    const api::internal::Core& api,
    const proto::ServerRequest serialized)
    : Signable(
          api,
          extract_nym(api, serialized),
          serialized.version(),
          "",
          "",
          api.Factory().Identifier(serialized.id()),
          serialized.has_signature()
              ? Signatures{std::make_shared<proto::Signature>(
                    serialized.signature())}
              : Signatures{})
    , initiator_((nym_) ? nym_->ID() : api.Factory().NymID().get())
    , server_(api.Factory().ServerID(serialized.server()))
    , type_(serialized.type())
    , number_(serialized.request())
    , include_nym_(Flag::Factory(false))
{
    Lock lock(lock_);
    init_serialized(lock);
}

Request::Request(const Request& rhs)
    : Signable(rhs)
    , initiator_(rhs.initiator_)
    , server_(rhs.server_)
    , type_(rhs.type_)
    , number_(rhs.number_)
    , include_nym_(Flag::Factory(rhs.include_nym_.get()))
{
}

auto Request::Contract() const -> proto::ServerRequest
{
    Lock lock(lock_);
    auto output = full_version(lock);

    return output;
}

auto Request::extract_nym(
    const api::internal::Core& api,
    const proto::ServerRequest serialized) -> Nym_p
{
    if (serialized.has_credentials()) {

        return api.Wallet().Nym(serialized.credentials());
    } else if (false == serialized.nym().empty()) {

        return api.Wallet().Nym(api.Factory().NymID(serialized.nym()));
    }

    return nullptr;
}

auto Request::full_version(const Lock& lock) const -> proto::ServerRequest
{
    auto contract = signature_version(lock);

    if (0 < signatures_.size()) {
        *(contract.mutable_signature()) = *(signatures_.front());
    }

    if (include_nym_.get() && bool(nym_)) {
        *contract.mutable_credentials() = nym_->asPublicNym();
    }

    return contract;
}

auto Request::GetID(const Lock& lock) const -> OTIdentifier
{
    return api_.Factory().Identifier(id_version(lock));
}

auto Request::id_version(const Lock& lock) const -> proto::ServerRequest
{
    proto::ServerRequest output{};
    output.set_version(version_);
    output.clear_id();  // Must be blank
    output.set_type(type_);
    output.set_nym(initiator_->str());
    output.set_server(server_->str());
    output.set_request(number_);
    output.clear_signature();  // Must be blank

    return output;
}

auto Request::Number() const -> RequestNumber
{
    Lock lock(lock_);

    return number_;
}

auto Request::Serialize() const -> OTData
{
    Lock lock(lock_);

    return api_.Factory().Data(full_version(lock));
}

auto Request::SetIncludeNym(const bool include, const PasswordPrompt& reason)
    -> bool
{
    Lock lock(lock_);

    if (include) {
        include_nym_->On();
    } else {
        include_nym_->Off();
    }

    return update_signature(lock, reason);
}

auto Request::signature_version(const Lock& lock) const -> proto::ServerRequest
{
    auto contract = id_version(lock);
    contract.set_id(id_->str());

    return contract;
}

auto Request::update_signature(const Lock& lock, const PasswordPrompt& reason)
    -> bool
{
    if (false == Signable::update_signature(lock, reason)) { return false; }

    bool success = false;
    signatures_.clear();
    auto serialized = signature_version(lock);
    auto& signature = *serialized.mutable_signature();
    success = nym_->Sign(
        serialized, crypto::SignatureRole::ServerRequest, signature, reason);

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
    bool validNym{false};

    if (nym_) { validNym = nym_->VerifyPseudonym(); }

    if (false == validNym) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid nym.").Flush();

        return false;
    }

    const bool validSyntax = proto::Validate(full_version(lock), VERBOSE);

    if (false == validSyntax) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid syntax.").Flush();

        return false;
    }

    if (1 != signatures_.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong number signatures.")
            .Flush();

        return false;
    }

    bool validSig{false};
    auto& signature = *signatures_.cbegin();

    if (signature) { validSig = verify_signature(lock, *signature); }

    if (false == validSig) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid signature.").Flush();

        return false;
    }

    return true;
}

auto Request::verify_signature(
    const Lock& lock,
    const proto::Signature& signature) const -> bool
{
    if (false == Signable::verify_signature(lock, signature)) { return false; }

    auto serialized = signature_version(lock);
    auto& sigProto = *serialized.mutable_signature();
    sigProto.CopyFrom(signature);

    return nym_->Verify(serialized, sigProto);
}
}  // namespace opentxs::otx::implementation
