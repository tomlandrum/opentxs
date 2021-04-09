// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"            // IWYU pragma: associated
#include "1_Internal.hpp"          // IWYU pragma: associated
#include "otx/consensus/Base.hpp"  // IWYU pragma: associated

#include <stdexcept>
#include <utility>

#include "internal/api/Api.hpp"
#include "internal/otx/OTX.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Ledger.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/NymFile.hpp"  // IWYU pragma: keep
#include "opentxs/core/String.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/SignatureRole.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/protobuf/Context.pb.h"
#include "opentxs/protobuf/Enums.pb.h"
#include "opentxs/protobuf/Signature.pb.h"
#include "opentxs/otx/Types.hpp"

#ifndef OT_MAX_ACK_NUMS
#define OT_MAX_ACK_NUMS 100
#endif

#define OT_METHOD "opentxs::otx::context::implementation::Context::"

namespace opentxs::otx::context::implementation
{
Base::Base(
    const api::internal::Core& api,
    const VersionNumber targetVersion,
    const Nym_p& local,
    const Nym_p& remote,
    const identifier::Server& server)
    : Signable(
          api,
          local,
          targetVersion,
          {},
          {},
          calculate_id(api, local, remote),
          {})
    , server_id_(server)
    , remote_nym_(remote)
    , available_transaction_numbers_()
    , issued_transaction_numbers_()
    , request_number_(0)
    , acknowledged_request_numbers_()
    , local_nymbox_hash_(Identifier::Factory())
    , remote_nymbox_hash_(Identifier::Factory())
    , target_version_(targetVersion)
{
}

Base::Base(
    const api::internal::Core& api,
    const VersionNumber targetVersion,
    const proto::Context& serialized,
    const Nym_p& local,
    const Nym_p& remote,
    const identifier::Server& server)
    : Signable(
          api,
          local,
          targetVersion,
          {},
          {},
          calculate_id(api, local, remote),
          serialized.has_signature()
              ? Signatures{std::make_shared<proto::Signature>(
                    serialized.signature())}
              : Signatures{})
    , server_id_(server)
    , remote_nym_(remote)
    , available_transaction_numbers_()
    , issued_transaction_numbers_()
    , request_number_(serialized.requestnumber())
    , acknowledged_request_numbers_()
    , local_nymbox_hash_(Identifier::Factory(serialized.localnymboxhash()))
    , remote_nymbox_hash_(Identifier::Factory(serialized.remotenymboxhash()))
    , target_version_(targetVersion)
{
    for (const auto& it : serialized.acknowledgedrequestnumber()) {
        acknowledged_request_numbers_.insert(it);
    }

    for (const auto& it : serialized.availabletransactionnumber()) {
        available_transaction_numbers_.insert(it);
    }

    for (const auto& it : serialized.issuedtransactionnumber()) {
        issued_transaction_numbers_.insert(it);
    }
}

auto Base::AcknowledgedNumbers() const -> std::set<RequestNumber>
{
    Lock lock(lock_);

    return acknowledged_request_numbers_;
}

auto Base::add_acknowledged_number(const Lock& lock, const RequestNumber req)
    -> bool
{
    OT_ASSERT(verify_write_lock(lock));

    auto output = acknowledged_request_numbers_.insert(req);

    while (OT_MAX_ACK_NUMS < acknowledged_request_numbers_.size()) {
        acknowledged_request_numbers_.erase(
            acknowledged_request_numbers_.begin());
    }

    return output.second;
}

auto Base::AddAcknowledgedNumber(const RequestNumber req) -> bool
{
    Lock lock(lock_);

    return add_acknowledged_number(lock, req);
}

auto Base::AvailableNumbers() const -> std::size_t
{
    return available_transaction_numbers_.size();
}

auto Base::calculate_id(
    const api::Core& api,
    const Nym_p& client,
    const Nym_p& server) noexcept(false) -> OTIdentifier
{
    if (false == (client && server)) {
        throw std::runtime_error("Invalid nym");
    }

    auto preimage = api.Factory().Data();
    preimage->Assign(client->ID());
    preimage.get() += server->ID();

    return api.Factory().Identifier(preimage->Bytes());
}

auto Base::consume_available(const Lock& lock, const TransactionNumber& number)
    -> bool
{
    OT_ASSERT(verify_write_lock(lock));

    LogVerbose(OT_METHOD)(__FUNCTION__)(": (")(type())(") ")(
        "Consuming number ")(number)
        .Flush();

    return 1 == available_transaction_numbers_.erase(number);
}

auto Base::consume_issued(const Lock& lock, const TransactionNumber& number)
    -> bool
{
    OT_ASSERT(verify_write_lock(lock));

    LogVerbose(OT_METHOD)(__FUNCTION__)(": (")(type())(") ")(
        "Consuming number ")(number)
        .Flush();

    if (0 < available_transaction_numbers_.count(number)) {
        LogDetail(OT_METHOD)(__FUNCTION__)(
            ": Consuming an issued number that was still available.")
            .Flush();

        available_transaction_numbers_.erase(number);
    }

    return 1 == issued_transaction_numbers_.erase(number);
}

auto Base::ConsumeAvailable(const TransactionNumber& number) -> bool
{
    Lock lock(lock_);

    return consume_available(lock, number);
}

auto Base::ConsumeIssued(const TransactionNumber& number) -> bool
{
    Lock lock(lock_);

    return consume_issued(lock, number);
}

auto Base::contract(const Lock& lock) const -> proto::Context
{
    OT_ASSERT(verify_write_lock(lock));

    auto output = serialize(lock);

    if (0 < signatures_.size()) {
        auto& sigProto = *output.mutable_signature();
        sigProto.CopyFrom(*signatures_.front());
    }

    return output;
}

// This method will remove entries from acknowledged_request_numbers_ if they
// are not on the provided set
void Base::finish_acknowledgements(
    const Lock& lock,
    const std::set<RequestNumber>& req)
{
    OT_ASSERT(verify_write_lock(lock));

    std::set<RequestNumber> toErase;

    for (const auto& number : acknowledged_request_numbers_) {
        if (0 == req.count(number)) { toErase.insert(number); }
    }

    for (const auto& it : toErase) { acknowledged_request_numbers_.erase(it); }
}

auto Base::GetID(const Lock& lock) const -> OTIdentifier
{
    OT_ASSERT(verify_write_lock(lock));

    try {
        return calculate_id(api_, nym_, remote_nym_);
    } catch (...) {
        return api_.Factory().Identifier();
    }
}

auto Base::HaveLocalNymboxHash() const -> bool
{
    return String::Factory(local_nymbox_hash_)->Exists();
}

auto Base::HaveRemoteNymboxHash() const -> bool
{
    return String::Factory(remote_nymbox_hash_)->Exists();
}

auto Base::IDVersion(const Lock& lock) const -> proto::Context
{
    OT_ASSERT(verify_write_lock(lock));

    auto output = proto::Context{};
    output.set_version(version_);

    switch (Type()) {
        case otx::ConsensusType::Server: {
            if (nym_) { output.set_localnym(nym_->ID().str()); }

            if (remote_nym_) { output.set_remotenym(remote_nym_->ID().str()); }

            output.set_localnymboxhash(
                String::Factory(local_nymbox_hash_)->Get());
            output.set_remotenymboxhash(
                String::Factory(remote_nymbox_hash_)->Get());
        } break;
        case otx::ConsensusType::Client: {
            if (nym_) { output.set_remotenym(nym_->ID().str()); }

            if (remote_nym_) { output.set_localnym(remote_nym_->ID().str()); }

            output.set_remotenymboxhash(
                String::Factory(local_nymbox_hash_)->Get());
            output.set_localnymboxhash(
                String::Factory(remote_nymbox_hash_)->Get());
        } break;
        default: {
            OT_FAIL;
        }
    }

    output.set_requestnumber(request_number_.load());

    for (const auto& it : available_transaction_numbers_) {
        output.add_availabletransactionnumber(it);
    }

    for (const auto& it : issued_transaction_numbers_) {
        output.add_issuedtransactionnumber(it);
    }

    return output;
}

auto Base::IncrementRequest() -> RequestNumber
{
    Lock lock(lock_);

    return ++request_number_;
}

auto Base::InitializeNymbox(const PasswordPrompt& reason) -> bool
{
    Lock lock(lock_);
    const auto& ownerNymID = client_nym_id(lock);
    auto nymbox{
        api_.Factory().Ledger(ownerNymID, server_nym_id(lock), server_id_)};

    if (false == bool(nymbox)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to instantiate nymbox for ")(ownerNymID)(".")
            .Flush();

        return false;
    }

    const auto generated = nymbox->GenerateLedger(
        ownerNymID, server_id_, ledgerType::nymbox, true);

    if (false == generated) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": (")(type())(") ")(
            "Unable to generate nymbox for ")(ownerNymID)(".")
            .Flush();

        return false;
    }

    nymbox->ReleaseSignatures();

    OT_ASSERT(nym_)

    if (false == nymbox->SignContract(*nym_, reason)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": (")(type())(") ")(
            "Unable to sign nymbox for ")(ownerNymID)(".")
            .Flush();

        return false;
    }

    if (false == nymbox->SaveContract()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": (")(type())(") ")(
            "Unable to serialize nymbox for ")(ownerNymID)(".")
            .Flush();

        return false;
    }

    if (false == nymbox->SaveNymbox(local_nymbox_hash_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": (")(type())(") ")(
            "Unable to save nymbox for ")(ownerNymID)
            .Flush();

        return false;
    }

    return true;
}

auto Base::insert_available_number(const TransactionNumber& number) -> bool
{
    Lock lock(lock_);

    return available_transaction_numbers_.insert(number).second;
}

auto Base::insert_issued_number(const TransactionNumber& number) -> bool
{
    Lock lock(lock_);

    return issued_transaction_numbers_.insert(number).second;
}

auto Base::issue_number(const Lock& lock, const TransactionNumber& number)
    -> bool
{
    OT_ASSERT(verify_write_lock(lock));

    issued_transaction_numbers_.insert(number);
    available_transaction_numbers_.insert(number);
    const bool issued = (1 == issued_transaction_numbers_.count(number));
    const bool available = (1 == available_transaction_numbers_.count(number));
    const bool output = issued && available;

    if (!output) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": (")(type())(") ")(
            "Failed to issue number ")(number)(".")
            .Flush();
        issued_transaction_numbers_.erase(number);
        available_transaction_numbers_.erase(number);
    }

    return output;
}

auto Base::IssuedNumbers() const -> std::set<TransactionNumber>
{
    Lock lock(lock_);

    return issued_transaction_numbers_;
}

auto Base::LegacyDataFolder() const -> std::string { return api_.DataFolder(); }

auto Base::LocalNymboxHash() const -> OTIdentifier
{
    Lock lock(lock_);

    return local_nymbox_hash_;
}

auto Base::mutable_Nymfile(const PasswordPrompt& reason)
    -> Editor<opentxs::NymFile>
{
    OT_ASSERT(nym_)

    return api_.Wallet().mutable_Nymfile(nym_->ID(), reason);
}

auto Base::Name() const -> std::string
{
    Lock lock(lock_);

    return String::Factory(id(lock))->Get();
}

auto Base::NymboxHashMatch() const -> bool
{
    Lock lock(lock_);

    if (!HaveLocalNymboxHash()) { return false; }

    if (!HaveRemoteNymboxHash()) { return false; }

    return (local_nymbox_hash_ == remote_nymbox_hash_);
}

auto Base::Nymfile(const PasswordPrompt& reason) const
    -> std::unique_ptr<const opentxs::NymFile>
{
    OT_ASSERT(nym_);

    return api_.Wallet().Nymfile(nym_->ID(), reason);
}

auto Base::recover_available_number(
    const Lock& lock,
    const TransactionNumber& number) -> bool
{
    OT_ASSERT(verify_write_lock(lock));

    if (0 == number) { return false; }

    const bool issued = 1 == issued_transaction_numbers_.count(number);

    if (!issued) { return false; }

    return available_transaction_numbers_.insert(number).second;
}

auto Base::RecoverAvailableNumber(const TransactionNumber& number) -> bool
{
    Lock lock(lock_);

    return recover_available_number(lock, number);
}

auto Base::Refresh(const PasswordPrompt& reason) -> proto::Context
{
    Lock lock(lock_);
    update_signature(lock, reason);

    return contract(lock);
}

auto Base::RemoteNym() const -> const identity::Nym&
{
    OT_ASSERT(remote_nym_);

    return *remote_nym_;
}

auto Base::RemoteNymboxHash() const -> OTIdentifier
{
    Lock lock(lock_);

    return remote_nymbox_hash_;
}

auto Base::remove_acknowledged_number(
    const Lock& lock,
    const std::set<RequestNumber>& req) -> bool
{
    OT_ASSERT(verify_write_lock(lock));

    std::size_t removed = 0;

    for (const auto& number : req) {
        removed += acknowledged_request_numbers_.erase(number);
    }

    return (0 < removed);
}

auto Base::RemoveAcknowledgedNumber(const std::set<RequestNumber>& req) -> bool
{
    Lock lock(lock_);

    return remove_acknowledged_number(lock, req);
}

auto Base::Request() const -> RequestNumber { return request_number_.load(); }

void Base::Reset()
{
    Lock lock(lock_);
    available_transaction_numbers_.clear();
    issued_transaction_numbers_.clear();
    request_number_.store(0);
}

auto Base::save(const Lock& lock, const PasswordPrompt& reason) -> bool
{
    OT_ASSERT(verify_write_lock(lock));

    if (false == UpdateSignature(lock, reason)) { return false; }
    if (false == ValidateContext(lock)) { return false; }

    return api_.Storage().Store(GetContract(lock));
}

auto Base::serialize(const Lock& lock, const otx::ConsensusType type) const
    -> proto::Context
{
    OT_ASSERT(verify_write_lock(lock));

    proto::Context output;
    output.set_version(version_);
    output.set_type(otx::internal::translate(type));

    if (nym_) { output.set_localnym(nym_->ID().str()); }

    if (remote_nym_) { output.set_remotenym(remote_nym_->ID().str()); }

    output.set_localnymboxhash(String::Factory(local_nymbox_hash_)->Get());
    output.set_remotenymboxhash(String::Factory(remote_nymbox_hash_)->Get());
    output.set_requestnumber(request_number_.load());

    for (const auto& it : acknowledged_request_numbers_) {
        output.add_acknowledgedrequestnumber(it);
    }

    for (const auto& it : available_transaction_numbers_) {
        output.add_availabletransactionnumber(it);
    }

    for (const auto& it : issued_transaction_numbers_) {
        output.add_issuedtransactionnumber(it);
    }

    return output;
}

auto Base::Serialize() const -> OTData
{
    return api_.Factory().Data(Serialized());
}

auto Base::Serialized() const -> proto::Context
{
    Lock lock(lock_);

    return contract(lock);
}

void Base::set_local_nymbox_hash(const Lock& lock, const Identifier& hash)
{
    OT_ASSERT(verify_write_lock(lock));

    local_nymbox_hash_ = Identifier::Factory(hash);
    LogVerbose(OT_METHOD)(__FUNCTION__)(": (")(type())(") ")(
        "Set local nymbox hash to: ")(local_nymbox_hash_->asHex())
        .Flush();
}

void Base::set_remote_nymbox_hash(const Lock& lock, const Identifier& hash)
{
    OT_ASSERT(verify_write_lock(lock));

    remote_nymbox_hash_ = Identifier::Factory(hash);
    LogVerbose(OT_METHOD)(__FUNCTION__)(": (")(type())(") ")(
        "Set remote nymbox hash to: ")(remote_nymbox_hash_->asHex())
        .Flush();
}

void Base::SetLocalNymboxHash(const Identifier& hash)
{
    Lock lock(lock_);
    set_local_nymbox_hash(lock, hash);
}

void Base::SetRemoteNymboxHash(const Identifier& hash)
{
    Lock lock(lock_);
    set_remote_nymbox_hash(lock, hash);
}

void Base::SetRequest(const RequestNumber req)
{
    Lock lock(lock_);

    request_number_.store(req);
}

auto Base::SigVersion(const Lock& lock) const -> proto::Context
{
    OT_ASSERT(verify_write_lock(lock));

    auto output = serialize(lock, Type());
    output.clear_signature();

    return output;
}

auto Base::update_signature(const Lock& lock, const PasswordPrompt& reason)
    -> bool
{
    OT_ASSERT(verify_write_lock(lock));

    if (!Signable::update_signature(lock, reason)) { return false; }

    update_version(target_version_);
    bool success = false;
    auto serialized = SigVersion(lock);
    auto& signature = *serialized.mutable_signature();
    success = nym_->Sign(
        serialized, crypto::SignatureRole::Context, signature, reason);

    if (success) {
        signatures_.emplace_front(new proto::Signature(signature));
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": (")(type())(") ")(
            "Failed to create signature.")
            .Flush();
    }

    return success;
}

auto Base::validate(const Lock& lock) const -> bool
{
    OT_ASSERT(verify_write_lock(lock));

    if (1 != signatures_.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Error: This context is not signed.")
            .Flush();

        return false;
    }

    return verify_signature(lock, *signatures_.front());
}

auto Base::verify_acknowledged_number(
    const Lock& lock,
    const RequestNumber& req) const -> bool
{
    OT_ASSERT(verify_write_lock(lock));

    return (0 < acknowledged_request_numbers_.count(req));
}

auto Base::verify_available_number(
    const Lock& lock,
    const TransactionNumber& number) const -> bool
{
    OT_ASSERT(verify_write_lock(lock));

    return (0 < available_transaction_numbers_.count(number));
}

auto Base::verify_issued_number(
    const Lock& lock,
    const TransactionNumber& number) const -> bool
{
    OT_ASSERT(verify_write_lock(lock));

    return (0 < issued_transaction_numbers_.count(number));
}

auto Base::verify_signature(const Lock& lock, const proto::Signature& signature)
    const -> bool
{
    OT_ASSERT(verify_write_lock(lock));

    if (!Signable::verify_signature(lock, signature)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": (")(type())(") ")(
            "Error: invalid signature.")
            .Flush();

        return false;
    }

    auto serialized = SigVersion(lock);
    auto& sigProto = *serialized.mutable_signature();
    sigProto.CopyFrom(signature);

    return nym_->Verify(serialized, sigProto);
}

auto Base::VerifyAcknowledgedNumber(const RequestNumber& req) const -> bool
{
    Lock lock(lock_);

    return verify_acknowledged_number(lock, req);
}

auto Base::VerifyAvailableNumber(const TransactionNumber& number) const -> bool
{
    Lock lock(lock_);

    return verify_available_number(lock, number);
}

auto Base::VerifyIssuedNumber(const TransactionNumber& number) const -> bool
{
    Lock lock(lock_);

    return verify_issued_number(lock, number);
}
}  // namespace opentxs::otx::context::implementation
