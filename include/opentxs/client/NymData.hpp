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
class OPENTXS_EXPORT NymData
{
public:
    NymData(const NymData&);
    NymData(NymData&&);

    identity::Nym::Serialized asPublicNym() const;
    std::string BestEmail() const;
    std::string BestPhoneNumber() const;
    std::string BestSocialMediaProfile(
        const contact::ContactItemType type) const;
    const opentxs::ContactData& Claims() const;
    bool DeleteClaim(const Identifier& id, const PasswordPrompt& reason);
    std::string EmailAddresses(bool active = true) const;
    bool HaveContract(
        const identifier::UnitDefinition& id,
        const contact::ContactItemType currency,
        const bool primary,
        const bool active) const;
    std::string Name() const;
    const identity::Nym& Nym() const;
    std::string PaymentCode(const contact::ContactItemType currency) const;
    std::string PhoneNumbers(bool active = true) const;
    std::string PreferredOTServer() const;
    std::string PrintContactData() const;
    std::string SocialMediaProfiles(
        const contact::ContactItemType type,
        bool active = true) const;
    std::set<contact::ContactItemType> SocialMediaProfileTypes() const;
    contact::ContactItemType Type() const;
    bool Valid() const;

    std::string AddChildKeyCredential(
        const Identifier& strMasterID,
        const NymParameters& nymParameters,
        const PasswordPrompt& reason);
    bool AddClaim(const Claim& claim, const PasswordPrompt& reason);
    bool AddContract(
        const std::string& instrumentDefinitionID,
        const contact::ContactItemType currency,
        const bool primary,
        const bool active,
        const PasswordPrompt& reason);
    bool AddEmail(
        const std::string& value,
        const bool primary,
        const bool active,
        const PasswordPrompt& reason);
    bool AddPaymentCode(
        const std::string& code,
        const contact::ContactItemType currency,
        const bool primary,
        const bool active,
        const PasswordPrompt& reason);
    bool AddPhoneNumber(
        const std::string& value,
        const bool primary,
        const bool active,
        const PasswordPrompt& reason);
    bool AddPreferredOTServer(
        const std::string& id,
        const bool primary,
        const PasswordPrompt& reason);
    bool AddSocialMediaProfile(
        const std::string& value,
        const contact::ContactItemType type,
        const bool primary,
        const bool active,
        const PasswordPrompt& reason);
    void Release();
    bool SetCommonName(const std::string& name, const PasswordPrompt& reason);
    bool SetContactData(
        const proto::ContactData& data,
        const PasswordPrompt& reason);
    bool SetContactData(const Space& data, const PasswordPrompt& reason);
    bool SetScope(
        const contact::ContactItemType type,
        const std::string& name,
        const bool primary,
        const PasswordPrompt& reason);

    ~NymData();

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
