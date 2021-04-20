// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <tuple>

#include "1_Internal.hpp"
#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "internal/api/client/Client.hpp"
#include "internal/contact/Contact.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/client/NymData.hpp"
#include "opentxs/contact/ContactData.hpp"
#include "opentxs/contact/ContactItem.hpp"
#include "opentxs/contact/ContactItemAttribute.hpp"
#include "opentxs/contact/ContactSectionName.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/identity/credential/Contact.hpp"
#include "opentxs/protobuf/Contact.hpp"

namespace
{
class Test_NymData : public ::testing::Test
{
public:
    const ot::api::client::internal::Manager& client_;
    ot::OTPasswordPrompt reason_;
    ot::NymData nymData_;

    static std::string ExpectedStringOutput(const std::uint32_t version)
    {
        return std::string{"Version "} + std::to_string(version) +
               std::string(" contact data\nSections found: 1\n- Section: "
                           "Scope, version: ") +
               std::to_string(version) +
               std::string{" containing 1 item(s).\n-- Item type: "
                           "\"Individual\", value: "
                           "\"testNym\", start: 0, end: 0, version: "} +
               std::to_string(version) +
               std::string{"\n--- Attributes: Active Primary \n"};
    }

    Test_NymData()
        : client_(dynamic_cast<const ot::api::client::internal::Manager&>(
              ot::Context().StartClient({}, 0)))
        , reason_(client_.Factory().PasswordPrompt(__FUNCTION__))
        , nymData_(client_.Wallet().mutable_Nym(
              client_.Wallet().Nym(reason_, "testNym")->ID(),
              reason_))
    {
    }
};

static const std::string paymentCode{
    "PM8TJKxypQfFUaHfSq59nn82EjdGU4SpHcp2ssa4GxPshtzoFtmnjfoRuHpvLiyASD7itH6auP"
    "C66jekGjnqToqS9ZJWWdf1c9L8x4iaFCQ2Gq5hMEFC"};
}  // namespace

TEST_F(Test_NymData, AddClaim)
{
    ot::Claim claim = std::make_tuple(
        std::string(""),
        ot::contact::internal::translate(
            ot::contact::ContactSectionName::Contract),
        ot::contact::internal::translate(ot::contact::ContactItemType::USD),
        std::string("claimValue"),
        NULL_START,
        NULL_END,
        std::set<std::uint32_t>{
            static_cast<uint32_t>(ot::contact::ContactItemAttribute::Active)});

    auto added = nymData_.AddClaim(claim, reason_);
    EXPECT_TRUE(added);
}

TEST_F(Test_NymData, AddContract)
{
    auto added = nymData_.AddContract(
        "", ot::contact::ContactItemType::USD, false, false, reason_);
    EXPECT_FALSE(added);

    const auto identifier1(ot::identifier::UnitDefinition::Factory(
        ot::identity::credential::Contact::ClaimID(
            client_,
            "testNym",
            ot::contact::ContactSectionName::Contract,
            ot::contact::ContactItemType::USD,
            NULL_START,
            NULL_END,
            "instrumentDefinitionID1",
            "")));

    added = nymData_.AddContract(
        identifier1->str(),
        ot::contact::ContactItemType::USD,
        false,
        false,
        reason_);
    EXPECT_TRUE(added);
}

TEST_F(Test_NymData, AddEmail)
{
    auto added = nymData_.AddEmail("email1", false, false, reason_);
    EXPECT_TRUE(added);

    added = nymData_.AddEmail("", false, false, reason_);
    EXPECT_FALSE(added);
}

TEST_F(Test_NymData, AddPaymentCode)
{
    auto added = nymData_.AddPaymentCode(
        "", ot::contact::ContactItemType::USD, false, false, reason_);
    EXPECT_FALSE(added);

    added = nymData_.AddPaymentCode(
        paymentCode, ot::contact::ContactItemType::USD, false, false, reason_);
    EXPECT_TRUE(added);
}

TEST_F(Test_NymData, AddPhoneNumber)
{
    auto added = nymData_.AddPhoneNumber("phone1", false, false, reason_);
    EXPECT_TRUE(added);

    added = nymData_.AddPhoneNumber("", false, false, reason_);
    EXPECT_FALSE(added);
}

TEST_F(Test_NymData, AddPreferredOTServer)
{
    const auto identifier(ot::identifier::Server::Factory(
        ot::identity::credential::Contact::ClaimID(
            client_,
            "testNym",
            ot::contact::ContactSectionName::Communication,
            ot::contact::ContactItemType::Opentxs,
            NULL_START,
            NULL_END,
            "localhost",
            "")));

    auto added =
        nymData_.AddPreferredOTServer(identifier->str(), false, reason_);
    EXPECT_TRUE(added);

    added = nymData_.AddPreferredOTServer("", false, reason_);
    EXPECT_FALSE(added);
}

TEST_F(Test_NymData, AddSocialMediaProfile)
{
    auto added = nymData_.AddSocialMediaProfile(
        "profile1",
        ot::contact::ContactItemType::Twitter,
        false,
        false,
        reason_);
    EXPECT_TRUE(added);

    added = nymData_.AddSocialMediaProfile(
        "", ot::contact::ContactItemType::Twitter, false, false, reason_);
    EXPECT_FALSE(added);
}

TEST_F(Test_NymData, BestEmail)
{
    auto added = nymData_.AddEmail("email1", false, false, reason_);
    EXPECT_TRUE(added);

    added = nymData_.AddEmail("email2", false, true, reason_);
    EXPECT_TRUE(added);

    std::string email = nymData_.BestEmail();
    // First email added is made primary.
    EXPECT_STREQ("email1", email.c_str());
}

TEST_F(Test_NymData, BestPhoneNumber)
{
    auto added = nymData_.AddPhoneNumber("phone1", false, false, reason_);
    EXPECT_TRUE(added);

    added = nymData_.AddPhoneNumber("phone2", false, true, reason_);
    EXPECT_TRUE(added);

    std::string phone = nymData_.BestPhoneNumber();
    // First phone number added is made primary.
    EXPECT_STREQ("phone1", phone.c_str());
}

TEST_F(Test_NymData, BestSocialMediaProfile)
{
    auto added = nymData_.AddSocialMediaProfile(
        "profile1", ot::contact::ContactItemType::Yahoo, false, false, reason_);
    EXPECT_TRUE(added);

    added = nymData_.AddSocialMediaProfile(
        "profile2", ot::contact::ContactItemType::Yahoo, false, true, reason_);
    EXPECT_TRUE(added);

    std::string profile =
        nymData_.BestSocialMediaProfile(ot::contact::ContactItemType::Yahoo);
    // First profile added is made primary.
    EXPECT_STREQ("profile1", profile.c_str());
}

TEST_F(Test_NymData, Claims)
{
    auto contactData = nymData_.Claims();
    const auto expected =
        ExpectedStringOutput(nymData_.Nym().ContactDataVersion());

    std::string output = contactData;
    EXPECT_TRUE(!output.empty());
    EXPECT_STREQ(expected.c_str(), output.c_str());
}

TEST_F(Test_NymData, DeleteClaim)
{
    ot::Claim claim = std::make_tuple(
        std::string(""),
        ot::contact::internal::translate(
            ot::contact::ContactSectionName::Contract),
        ot::contact::internal::translate(ot::contact::ContactItemType::USD),
        std::string("claimValue"),
        NULL_START,
        NULL_END,
        std::set<std::uint32_t>{
            static_cast<uint32_t>(ot::contact::ContactItemAttribute::Active)});

    auto added = nymData_.AddClaim(claim, reason_);
    ASSERT_TRUE(added);

    const auto identifier(ot::identifier::UnitDefinition::Factory(
        ot::identity::credential::Contact::ClaimID(
            client_,
            "testNym",
            ot::contact::ContactSectionName::Contract,
            ot::contact::ContactItemType::USD,
            NULL_START,
            NULL_END,
            "claimValue",
            "")));
    auto deleted = nymData_.DeleteClaim(identifier, reason_);
    EXPECT_TRUE(deleted);
}

TEST_F(Test_NymData, EmailAddresses)
{
    auto added = nymData_.AddEmail("email1", false, false, reason_);
    EXPECT_TRUE(added);

    added = nymData_.AddEmail("email2", false, false, reason_);
    EXPECT_TRUE(added);

    added = nymData_.AddEmail("email3", true, false, reason_);
    EXPECT_TRUE(added);

    auto emails = nymData_.EmailAddresses(false);
    EXPECT_TRUE(
        emails.find("email1") != std::string::npos &&
        emails.find("email2") != std::string::npos &&
        emails.find("email3") != std::string::npos);

    emails = nymData_.EmailAddresses();
    // First email added is made primary and active.
    EXPECT_TRUE(
        emails.find("email1") != std::string::npos &&
        emails.find("email3") != std::string::npos);
    EXPECT_TRUE(emails.find("email2") == std::string::npos);
}

TEST_F(Test_NymData, HaveContract)
{
    const auto identifier1(ot::identifier::UnitDefinition::Factory(
        ot::identity::credential::Contact::ClaimID(
            client_,
            "testNym",
            ot::contact::ContactSectionName::Contract,
            ot::contact::ContactItemType::USD,
            NULL_START,
            NULL_END,
            "instrumentDefinitionID1",
            "")));

    auto added = nymData_.AddContract(
        identifier1->str(),
        ot::contact::ContactItemType::USD,
        false,
        false,
        reason_);
    ASSERT_TRUE(added);

    auto haveContract = nymData_.HaveContract(
        identifier1, ot::contact::ContactItemType::USD, true, true);
    EXPECT_TRUE(haveContract);

    haveContract = nymData_.HaveContract(
        identifier1, ot::contact::ContactItemType::USD, true, false);
    EXPECT_TRUE(haveContract);

    haveContract = nymData_.HaveContract(
        identifier1, ot::contact::ContactItemType::USD, false, true);
    EXPECT_TRUE(haveContract);

    haveContract = nymData_.HaveContract(
        identifier1, ot::contact::ContactItemType::USD, false, false);
    EXPECT_TRUE(haveContract);

    const auto identifier2(ot::identifier::UnitDefinition::Factory(
        ot::identity::credential::Contact::ClaimID(
            client_,
            "testNym",
            ot::contact::ContactSectionName::Contract,
            ot::contact::ContactItemType::USD,
            NULL_START,
            NULL_END,
            "instrumentDefinitionID2",
            "")));

    added = nymData_.AddContract(
        identifier2->str(),
        ot::contact::ContactItemType::USD,
        false,
        false,
        reason_);
    ASSERT_TRUE(added);

    haveContract = nymData_.HaveContract(
        identifier2, ot::contact::ContactItemType::USD, false, false);
    EXPECT_TRUE(haveContract);

    haveContract = nymData_.HaveContract(
        identifier2, ot::contact::ContactItemType::USD, true, false);
    EXPECT_FALSE(haveContract);

    haveContract = nymData_.HaveContract(
        identifier2, ot::contact::ContactItemType::USD, false, true);
    EXPECT_FALSE(haveContract);

    haveContract = nymData_.HaveContract(
        identifier2, ot::contact::ContactItemType::USD, true, true);
    EXPECT_FALSE(haveContract);
}

TEST_F(Test_NymData, Name) { EXPECT_STREQ("testNym", nymData_.Name().c_str()); }

TEST_F(Test_NymData, Nym)
{
    EXPECT_STREQ("testNym", nymData_.Nym().Name().c_str());
}

TEST_F(Test_NymData, PaymentCode)
{
    auto added = nymData_.AddPaymentCode(
        paymentCode, ot::contact::ContactItemType::BTC, true, true, reason_);
    ASSERT_TRUE(added);

    auto paymentcode = nymData_.PaymentCode(ot::contact::ContactItemType::BTC);
    EXPECT_TRUE(!paymentcode.empty());
    EXPECT_STREQ(paymentCode.c_str(), paymentcode.c_str());

    paymentcode = nymData_.PaymentCode(ot::contact::ContactItemType::USD);
    EXPECT_TRUE(paymentcode.empty());
}

TEST_F(Test_NymData, PhoneNumbers)
{
    auto added = nymData_.AddPhoneNumber("phone1", false, false, reason_);
    ASSERT_TRUE(added);

    added = nymData_.AddPhoneNumber("phone2", false, false, reason_);
    ASSERT_TRUE(added);

    added = nymData_.AddPhoneNumber("phone3", true, false, reason_);
    ASSERT_TRUE(added);

    auto phones = nymData_.PhoneNumbers(false);
    EXPECT_TRUE(
        phones.find("phone1") != std::string::npos &&
        phones.find("phone2") != std::string::npos &&
        phones.find("phone3") != std::string::npos);

    phones = nymData_.PhoneNumbers();
    // First phone number added is made primary and active.
    EXPECT_TRUE(
        phones.find("phone1") != std::string::npos &&
        phones.find("phone3") != std::string::npos);
    EXPECT_TRUE(phones.find("phone2") == std::string::npos);
}

TEST_F(Test_NymData, PreferredOTServer)
{
    auto preferred = nymData_.PreferredOTServer();
    EXPECT_TRUE(preferred.empty());

    const auto identifier(ot::identifier::Server::Factory(
        ot::identity::credential::Contact::ClaimID(
            client_,
            "testNym",
            ot::contact::ContactSectionName::Communication,
            ot::contact::ContactItemType::Opentxs,
            NULL_START,
            NULL_END,
            "localhost",
            "")));
    auto added =
        nymData_.AddPreferredOTServer(identifier->str(), true, reason_);
    EXPECT_TRUE(added);

    preferred = nymData_.PreferredOTServer();
    EXPECT_TRUE(!preferred.empty());
    EXPECT_STREQ(identifier->str().c_str(), preferred.c_str());
}

TEST_F(Test_NymData, PrintContactData)
{
    const auto& text = nymData_.PrintContactData();
    const auto expected =
        ExpectedStringOutput(nymData_.Nym().ContactDataVersion());

    EXPECT_STREQ(expected.c_str(), text.c_str());
}

TEST_F(Test_NymData, SetContactData)
{
    const ot::ContactData contactData(
        client_,
        std::string("contactData"),
        nymData_.Nym().ContactDataVersion(),
        nymData_.Nym().ContactDataVersion(),
        {});

    auto bytes = ot::Space{};
    EXPECT_TRUE(contactData.Serialize(ot::writer(bytes)));
    auto set = nymData_.SetContactData(bytes, reason_);
    EXPECT_TRUE(set);
}

TEST_F(Test_NymData, SetScope)
{
    auto set = nymData_.SetScope(
        ot::contact::ContactItemType::Organization,
        "organizationScope",
        true,
        reason_);
    EXPECT_TRUE(set);

    set = nymData_.SetScope(
        ot::contact::ContactItemType::Business,
        "businessScope",
        false,
        reason_);
    EXPECT_TRUE(set);
}

TEST_F(Test_NymData, SocialMediaProfiles)
{
    auto added = nymData_.AddSocialMediaProfile(
        "profile1",
        ot::contact::ContactItemType::Facebook,
        false,
        false,
        reason_);
    EXPECT_TRUE(added);

    added = nymData_.AddSocialMediaProfile(
        "profile2",
        ot::contact::ContactItemType::Facebook,
        false,
        false,
        reason_);
    EXPECT_TRUE(added);

    added = nymData_.AddSocialMediaProfile(
        "profile3",
        ot::contact::ContactItemType::Facebook,
        true,
        false,
        reason_);
    EXPECT_TRUE(added);

    auto profiles = nymData_.SocialMediaProfiles(
        ot::contact::ContactItemType::Facebook, false);
    EXPECT_TRUE(
        profiles.find("profile1") != std::string::npos &&
        profiles.find("profile2") != std::string::npos &&
        profiles.find("profile3") != std::string::npos);

    profiles =
        nymData_.SocialMediaProfiles(ot::contact::ContactItemType::Facebook);
    // First profile added is made primary and active.
    EXPECT_TRUE(
        profiles.find("profile1") != std::string::npos &&
        profiles.find("profile3") != std::string::npos);
    EXPECT_TRUE(profiles.find("profile2") == std::string::npos);
}

TEST_F(Test_NymData, SocialMediaProfileTypes)
{
    std::set<ot::proto::ContactItemType> profileTypes =
        ot::proto::AllowedItemTypes().at(ot::proto::ContactSectionVersion(
            CONTACT_CONTACT_DATA_VERSION,
            ot::contact::internal::translate(
                ot::contact::ContactSectionName::Profile)));

    std::set<ot::contact::ContactItemType> output;
    std::transform(
        profileTypes.begin(),
        profileTypes.end(),
        std::inserter(output, output.end()),
        [](ot::proto::ContactItemType itemtype)
            -> ot::contact::ContactItemType {
            return ot::contact::internal::translate(itemtype);
        });

    EXPECT_EQ(output, nymData_.SocialMediaProfileTypes());
}

TEST_F(Test_NymData, Type)
{
    EXPECT_EQ(ot::contact::ContactItemType::Individual, nymData_.Type());
}

TEST_F(Test_NymData, Valid) { EXPECT_TRUE(nymData_.Valid()); }
