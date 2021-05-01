// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                // IWYU pragma: associated
#include "1_Internal.hpp"              // IWYU pragma: associated
#include "opentxs/client/NymData.hpp"  // IWYU pragma: associated

#include <utility>

#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.tpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/contact/Contact.hpp"
#include "opentxs/contact/ContactData.hpp"
#include "opentxs/contact/ContactItem.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/protobuf/Nym.pb.h"  // IWYU pragma: keep

#define OT_METHOD "opentxs::NymData::"

namespace opentxs
{
NymData::NymData(
    const api::Factory& factory,
    std::mutex& objectMutex,
    const std::shared_ptr<identity::Nym>& nym,
    LockedSave save)
    : factory_{factory}
    , object_lock_{new Lock(objectMutex)}
    , locked_save_callback_{new LockedSave(save)}
    , nym_(nym)
{
    OT_ASSERT(object_lock_);
    OT_ASSERT(locked_save_callback_);
}

NymData::NymData(NymData&& rhs)
    : factory_{rhs.factory_}
    , object_lock_(std::move(rhs.object_lock_))
    , locked_save_callback_(std::move(rhs.locked_save_callback_))
    , nym_(std::move(rhs.nym_))
{
}

// This constructor is only used by Swig.  Swig doesn't support move
// constructors, so this copy constructor implements move semantics.
NymData::NymData(const NymData& rhs)
    : NymData(std::move(const_cast<NymData&>(rhs)))
{
}

auto NymData::AddChildKeyCredential(
    const Identifier& strMasterID,
    const NymParameters& nymParameters,
    const PasswordPrompt& reason) -> std::string
{
    return nym().AddChildKeyCredential(strMasterID, nymParameters, reason);
}

auto NymData::AddClaim(const Claim& claim, const PasswordPrompt& reason) -> bool
{
    return nym().AddClaim(claim, reason);
}

auto NymData::DeleteClaim(const Identifier& id, const PasswordPrompt& reason)
    -> bool
{
    return nym().DeleteClaim(id, reason);
}

auto NymData::AddContract(
    const std::string& instrumentDefinitionID,
    const contact::ContactItemType currency,
    const bool primary,
    const bool active,
    const PasswordPrompt& reason) -> bool
{
    auto id = factory_.UnitID(instrumentDefinitionID);

    if (id->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Invalid instrument definition id.")
            .Flush();

        return false;
    }

    return nym().AddContract(id, currency, reason, primary, active);
}

auto NymData::AddEmail(
    const std::string& value,
    const bool primary,
    const bool active,
    const PasswordPrompt& reason) -> bool
{
    return nym().AddEmail(value, reason, primary, active);
}

auto NymData::AddPaymentCode(
    const std::string& code,
    const contact::ContactItemType currency,
    const bool primary,
    const bool active,
    const PasswordPrompt& reason) -> bool
{
    auto paymentCode = factory_.PaymentCode(code);

    if (false == paymentCode->Valid()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid payment code.").Flush();

        return false;
    }

    return nym().AddPaymentCode(paymentCode, currency, reason, primary, active);
}

auto NymData::AddPhoneNumber(
    const std::string& value,
    const bool primary,
    const bool active,
    const PasswordPrompt& reason) -> bool
{
    return nym().AddPhoneNumber(value, reason, primary, active);
}

auto NymData::AddPreferredOTServer(
    const std::string& id,
    const bool primary,
    const PasswordPrompt& reason) -> bool
{
    if (id.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid server id.").Flush();

        return false;
    }

    return nym().AddPreferredOTServer(factory_.ServerID(id), reason, primary);
}

auto NymData::AddSocialMediaProfile(
    const std::string& value,
    const contact::ContactItemType type,
    const bool primary,
    const bool active,
    const PasswordPrompt& reason) -> bool
{
    return nym().AddSocialMediaProfile(value, type, reason, primary, active);
}

auto NymData::asPublicNym() const -> identity::Nym::Serialized
{
    return nym().asPublicNym();
}

auto NymData::BestEmail() const -> std::string { return nym().BestEmail(); }

auto NymData::BestPhoneNumber() const -> std::string
{
    return nym().BestPhoneNumber();
}

auto NymData::BestSocialMediaProfile(const contact::ContactItemType type) const
    -> std::string
{
    return nym().BestSocialMediaProfile(type);
}

auto NymData::Claims() const -> const opentxs::ContactData&
{
    return nym().Claims();
}

auto NymData::data() const -> const ContactData& { return nym().Claims(); }

auto NymData::EmailAddresses(bool active) const -> std::string
{
    return nym().EmailAddresses(active);
}

auto NymData::HaveContract(
    const identifier::UnitDefinition& instrumentDefinitionID,
    const contact::ContactItemType currency,
    const bool primary,
    const bool active) const -> bool
{
    OT_ASSERT(nym_);

    const auto contracts = nym().Contracts(currency, active);

    if (0 == contracts.size()) { return false; }

    const auto& data = nym().Claims();

    for (const auto& id : contracts) {
        const auto& claim = data.Claim(id);

        OT_ASSERT(claim);

        const auto value = factory_.UnitID(claim->Value());

        if (false == (instrumentDefinitionID == value)) { continue; }

        if ((false == primary) || claim->isPrimary()) { return true; }
    }

    return false;
}

auto NymData::Name() const -> std::string { return nym().Name(); }

auto NymData::Nym() const -> const identity::Nym& { return nym(); }

auto NymData::nym() -> identity::Nym&
{
    OT_ASSERT(nym_);

    return *nym_;
}

auto NymData::nym() const -> const identity::Nym&
{
    OT_ASSERT(nym_);

    return *nym_;
}

auto NymData::PaymentCode(const contact::ContactItemType currency) const
    -> std::string
{
    return Contact::PaymentCode(data(), currency);
}

auto NymData::PhoneNumbers(bool active) const -> std::string
{
    return nym().PhoneNumbers(active);
}

auto NymData::PreferredOTServer() const -> std::string
{
    return data().PreferredOTServer()->str();
}

auto NymData::PrintContactData() const -> std::string
{
    return ContactData::PrintContactData(data().Serialize(true));
}

void NymData::Release() { release(); }

void NymData::release()
{
    if (locked_save_callback_) {
        auto callback = *locked_save_callback_;
        callback(this, *object_lock_);
    }

    locked_save_callback_.reset();

    if (object_lock_) {
        object_lock_->unlock();
        object_lock_.reset();
    }

    nym_.reset();
}

auto NymData::SetCommonName(
    const std::string& name,
    const PasswordPrompt& reason) -> bool
{
    return nym().SetCommonName(name, reason);
}

auto NymData::SetContactData(
    const proto::ContactData& data,
    const PasswordPrompt& reason) -> bool
{
    return nym().SetContactData(data, reason);
}

auto NymData::SetContactData(
    const ReadView& bytes,
    const PasswordPrompt& reason) -> bool
{
    auto data = proto::Factory<proto::ContactData>(bytes);
    return SetContactData(data, reason);
}

auto NymData::SetScope(
    const contact::ContactItemType type,
    const std::string& name,
    const bool primary,
    const PasswordPrompt& reason) -> bool
{
    return nym().SetScope(type, name, reason, primary);
}

auto NymData::SocialMediaProfiles(
    const contact::ContactItemType type,
    bool active) const -> std::string
{
    return nym().SocialMediaProfiles(type, active);
}

auto NymData::SocialMediaProfileTypes() const
    -> std::set<contact::ContactItemType>
{
    return nym().SocialMediaProfileTypes();
}

auto NymData::Type() const -> contact::ContactItemType { return data().Type(); }

auto NymData::Valid() const -> bool { return bool(nym_); }

NymData::~NymData() { release(); }
}  // namespace opentxs
