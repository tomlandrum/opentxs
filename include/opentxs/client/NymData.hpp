// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CLIENT_NYMDATA_HPP
#define OPENTXS_CLIENT_NYMDATA_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <set>
#include <string>
#include <vector>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/contact/Types.hpp"
#include "opentxs/identity/Nym.hpp"

#ifdef SWIG
// clang-format off
%template(ProtoTypeList) std::vector<int>;
%extend opentxs::NymData {
    std::string BestSocialMediaProfile(const int type) const
    {
        return $self->BestSocialMediaProfile(
            static_cast<opentxs::contact::ContactItemType>(type));
    }
    bool HaveContract(
        const std::string& id,
        const int currency,
        const bool primary,
        const bool active) const
    {
        return $self->HaveContract(
            opentxs::identifier::UnitDefinition::Factory(id),
            static_cast<opentxs::contact::ContactItemType>(currency),
            primary,
            active);
    }
    std::string PaymentCode(const int currency) const
    {
        return $self->PaymentCode(
            static_cast<opentxs::contact::ContactItemType>(currency));
    }
    std::string SocialMediaProfiles(
        const int type,
        bool active = true) const
    {
        return $self->SocialMediaProfiles(
            static_cast<opentxs::contact::ContactItemType>(type), active);
    }
    const std::vector<int> SocialMediaProfileTypes() const
    {
        const auto& types = $self->Nym().SocialMediaProfileTypes();

        std::vector<int> output;
        std::transform(
            types.begin(),
            types.end(),
            std::inserter(output, output.end()),
            [](opentxs::contact::ContactItemType type) -> int {
                return static_cast<int>(type);
            });

        return output;
    }
    int Type() const
    {
        return static_cast<int>($self->Type());
    }
    bool AddContract(
        const std::string& instrumentDefinitionID,
        const int currency,
        const bool primary,
        const bool active,
        const PasswordPrompt& reason)
    {
        return $self->AddContract(
            instrumentDefinitionID,
            static_cast<opentxs::contact::ContactItemType>(currency),
            primary,
            active,
            reason);
    }
    bool AddPaymentCode(
        const std::string& code,
        const int currency,
        const bool primary,
        const bool active,
        const PasswordPrompt& reason)
    {
        return $self->AddPaymentCode(
            code,
            static_cast<opentxs::contact::ContactItemType>(currency),
            primary,
            active,
            reason);
    }
    bool AddSocialMediaProfile(
        const std::string& value,
        const int type,
        const bool primary,
        const bool active,
        const PasswordPrompt& reason)
    {
        return $self->AddSocialMediaProfile(
            value,
            static_cast<opentxs::contact::ContactItemType>(type),
            primary,
            active, reason);
    }
    bool SetScope(
        const int type,
        const std::string& name,
        const bool primary,
        const PasswordPrompt& reason)
    {
        return $self->SetScope(
            static_cast<opentxs::contact::ContactItemType>(type),
            name,
            primary,
            reason);
    }
}

%ignore opentxs::NymData::AddChildKeyCredential;
%ignore opentxs::NymData::AddClaim;
%ignore opentxs::NymData::AddContract;
%ignore opentxs::NymData::AddPaymentCode;
%ignore opentxs::NymData::AddSocialMediaProfile;
%ignore opentxs::NymData::asPublicNym;
%ignore opentxs::NymData::BestSocialMediaProfile;
%ignore opentxs::NymData::Claims;
%ignore opentxs::NymData::DeleteClaim;
%ignore opentxs::NymData::HaveContract;
%ignore opentxs::NymData::Nym;
%ignore opentxs::NymData::PaymentCode;
%ignore opentxs::NymData::SetContactData;
%ignore opentxs::NymData::SetScope;
%ignore opentxs::NymData::SocialMediaProfiles;
%ignore opentxs::NymData::SocialMediaProfileTypes;
%ignore opentxs::NymData::Type;
%ignore opentxs::NymData::NymData(NymData&&);
// clang-format on
#endif

namespace opentxs
{
namespace api
{
class Factory;

namespace implementation
{
class Wallet;
}  // namespace implementation
}  // namespace api

namespace identifier
{
class UnitDefinition;
}  // namespace identifier

namespace proto
{
class ContactData;
}  // namespace proto

class ContactData;
class Identifier;
class NymParameters;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs
{
class NymData
{
public:
    OPENTXS_EXPORT NymData(const NymData&);
    OPENTXS_EXPORT NymData(NymData&&);

    OPENTXS_EXPORT identity::Nym::Serialized asPublicNym() const;
    OPENTXS_EXPORT std::string BestEmail() const;
    OPENTXS_EXPORT std::string BestPhoneNumber() const;
    OPENTXS_EXPORT std::string BestSocialMediaProfile(
        const contact::ContactItemType type) const;
    OPENTXS_EXPORT const opentxs::ContactData& Claims() const;
    OPENTXS_EXPORT bool DeleteClaim(
        const Identifier& id,
        const PasswordPrompt& reason);
    OPENTXS_EXPORT std::string EmailAddresses(bool active = true) const;
    OPENTXS_EXPORT bool HaveContract(
        const identifier::UnitDefinition& id,
        const contact::ContactItemType currency,
        const bool primary,
        const bool active) const;
    OPENTXS_EXPORT std::string Name() const;
    OPENTXS_EXPORT const identity::Nym& Nym() const;
    OPENTXS_EXPORT std::string PaymentCode(
        const contact::ContactItemType currency) const;
    OPENTXS_EXPORT std::string PhoneNumbers(bool active = true) const;
    OPENTXS_EXPORT std::string PreferredOTServer() const;
    OPENTXS_EXPORT std::string PrintContactData() const;
    OPENTXS_EXPORT std::string SocialMediaProfiles(
        const contact::ContactItemType type,
        bool active = true) const;
    OPENTXS_EXPORT std::set<contact::ContactItemType> SocialMediaProfileTypes()
        const;
    OPENTXS_EXPORT contact::ContactItemType Type() const;
    OPENTXS_EXPORT bool Valid() const;

    OPENTXS_EXPORT std::string AddChildKeyCredential(
        const Identifier& strMasterID,
        const NymParameters& nymParameters,
        const PasswordPrompt& reason);
    OPENTXS_EXPORT bool AddClaim(
        const Claim& claim,
        const PasswordPrompt& reason);
    OPENTXS_EXPORT bool AddContract(
        const std::string& instrumentDefinitionID,
        const contact::ContactItemType currency,
        const bool primary,
        const bool active,
        const PasswordPrompt& reason);
    OPENTXS_EXPORT bool AddEmail(
        const std::string& value,
        const bool primary,
        const bool active,
        const PasswordPrompt& reason);
    OPENTXS_EXPORT bool AddPaymentCode(
        const std::string& code,
        const contact::ContactItemType currency,
        const bool primary,
        const bool active,
        const PasswordPrompt& reason);
    OPENTXS_EXPORT bool AddPhoneNumber(
        const std::string& value,
        const bool primary,
        const bool active,
        const PasswordPrompt& reason);
    OPENTXS_EXPORT bool AddPreferredOTServer(
        const std::string& id,
        const bool primary,
        const PasswordPrompt& reason);
    OPENTXS_EXPORT bool AddSocialMediaProfile(
        const std::string& value,
        const contact::ContactItemType type,
        const bool primary,
        const bool active,
        const PasswordPrompt& reason);
    OPENTXS_EXPORT void Release();
    OPENTXS_EXPORT bool SetCommonName(
        const std::string& name,
        const PasswordPrompt& reason);
    OPENTXS_EXPORT bool SetContactData(
        const proto::ContactData& data,
        const PasswordPrompt& reason);
    OPENTXS_EXPORT bool SetScope(
        const contact::ContactItemType type,
        const std::string& name,
        const bool primary,
        const PasswordPrompt& reason);

    OPENTXS_EXPORT ~NymData();

private:
    friend api::implementation::Wallet;

    using Lock = std::unique_lock<std::mutex>;
    using LockedSave = std::function<void(NymData*, Lock&)>;

    const api::Factory& factory_;
    std::unique_ptr<Lock> object_lock_;
    std::unique_ptr<LockedSave> locked_save_callback_;

    std::shared_ptr<identity::Nym> nym_;

    const ContactData& data() const;

    const identity::Nym& nym() const;
    identity::Nym& nym();

    void release();

    NymData(
        const api::Factory& factory,
        std::mutex& objectMutex,
        const std::shared_ptr<identity::Nym>& nym,
        LockedSave save);
    NymData() = delete;
    NymData& operator=(const NymData&) = delete;
    NymData& operator=(NymData&&) = delete;
};
}  // namespace opentxs
#endif
