// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <ctime>
#include <string>

#include "1_Internal.hpp"
#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "internal/api/client/Client.hpp"
#include "internal/contact/Contact.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/contact/ContactItem.hpp"
#include "opentxs/contact/ContactItemType.hpp"
#include "opentxs/contact/ContactItemAttribute.hpp"
#include "opentxs/contact/ContactSectionName.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/identity/credential/Contact.hpp"
#include "opentxs/protobuf/ContactItem.pb.h"

namespace
{

class Test_ContactItem : public ::testing::Test
{
public:
    Test_ContactItem()
        : api_(dynamic_cast<const ot::api::client::internal::Manager&>(
              ot::Context().StartClient({}, 0)))
        , contactItem_(
              api_,
              std::string("testNym"),
              CONTACT_CONTACT_DATA_VERSION,
              CONTACT_CONTACT_DATA_VERSION,
              ot::contact::ContactSectionName::Identifier,
              ot::contact::ContactItemType::Employee,
              std::string("testValue"),
              {ot::contact::ContactItemAttribute::Active},
              NULL_START,
              NULL_END,
              "")
    {
    }

    const ot::api::client::internal::Manager& api_;
    const ot::ContactItem contactItem_;
};

}  // namespace

TEST_F(Test_ContactItem, first_constructor)
{
    const ot::ContactItem contactItem1(
        api_,
        std::string("testContactItemNym"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::Employee,
        std::string("testValue"),
        {ot::contact::ContactItemAttribute::Active},
        NULL_START,
        NULL_END,
        "");

    const ot::OTIdentifier identifier(
        ot::Identifier::Factory(ot::identity::credential::Contact::ClaimID(
            api_,
            "testContactItemNym",
            ot::contact::ContactSectionName::Identifier,
            ot::contact::ContactItemType::Employee,
            NULL_START,
            NULL_END,
            "testValue",
            "")));
    ASSERT_EQ(identifier, contactItem1.ID());
    ASSERT_EQ(CONTACT_CONTACT_DATA_VERSION, contactItem1.Version());
    ASSERT_EQ(
        ot::contact::ContactSectionName::Identifier, contactItem1.Section());
    ASSERT_EQ(ot::contact::ContactItemType::Employee, contactItem1.Type());
    ASSERT_EQ("testValue", contactItem1.Value());
    ASSERT_EQ(contactItem1.Start(), NULL_START);
    ASSERT_EQ(contactItem1.End(), NULL_END);

    ASSERT_TRUE(contactItem1.isActive());
    ASSERT_FALSE(contactItem1.isLocal());
    ASSERT_FALSE(contactItem1.isPrimary());
}

TEST_F(Test_ContactItem, first_constructor_different_versions)
{
    const ot::ContactItem contactItem1(
        api_,
        std::string("testContactItemNym"),
        CONTACT_CONTACT_DATA_VERSION - 1,  // previous version
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::Employee,
        std::string("testValue"),
        {ot::contact::ContactItemAttribute::Active},
        NULL_START,
        NULL_END,
        "");
    ASSERT_EQ(CONTACT_CONTACT_DATA_VERSION, contactItem1.Version());
}

TEST_F(Test_ContactItem, second_constructor)
{
    const ot::ContactItem contactItem1(
        api_,
        std::string("testContactItemNym"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::Claim(
            "",
            ot::contact::internal::translate(
                ot::contact::ContactSectionName::Identifier),
            ot::contact::internal::translate(
                ot::contact::ContactItemType::Employee),
            "testValue",
            NULL_START,
            NULL_END,
            {static_cast<uint32_t>(
                ot::contact::ContactItemAttribute::Active)}));

    const ot::OTIdentifier identifier(
        ot::Identifier::Factory(ot::identity::credential::Contact::ClaimID(
            api_,
            "testContactItemNym",
            ot::contact::ContactSectionName::Identifier,
            ot::contact::ContactItemType::Employee,
            NULL_START,
            NULL_END,
            "testValue",
            "")));
    ASSERT_EQ(identifier, contactItem1.ID());
    ASSERT_EQ(CONTACT_CONTACT_DATA_VERSION, contactItem1.Version());
    ASSERT_EQ(
        ot::contact::ContactSectionName::Identifier, contactItem1.Section());
    ASSERT_EQ(ot::contact::ContactItemType::Employee, contactItem1.Type());
    ASSERT_EQ("testValue", contactItem1.Value());
    ASSERT_EQ(contactItem1.Start(), NULL_START);
    ASSERT_EQ(contactItem1.End(), NULL_END);

    ASSERT_TRUE(contactItem1.isActive());
    ASSERT_FALSE(contactItem1.isLocal());
    ASSERT_FALSE(contactItem1.isPrimary());
}

TEST_F(Test_ContactItem, third_constructor)
{
    ot::proto::ContactItem data;
    data.set_version(CONTACT_CONTACT_DATA_VERSION);
    data.set_type(ot::contact::internal::translate(
        ot::contact::ContactItemType::Employee));
    data.set_value("testValue");
    data.add_attribute(ot::contact::internal::translate(
        ot::contact::ContactItemAttribute::Active));
    data.set_start(NULL_START);
    data.set_end(NULL_END);

    const ot::ContactItem contactItem1(
        api_,
        std::string("testContactItemNym"),
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        data);

    const ot::OTIdentifier identifier(
        ot::Identifier::Factory(ot::identity::credential::Contact::ClaimID(
            api_,
            "testContactItemNym",
            ot::contact::ContactSectionName::Identifier,
            ot::contact::ContactItemType::Employee,
            NULL_START,
            NULL_END,
            "testValue",
            "")));
    ASSERT_EQ(identifier, contactItem1.ID());
    ASSERT_EQ(CONTACT_CONTACT_DATA_VERSION, contactItem1.Version());
    ASSERT_EQ(
        ot::contact::ContactSectionName::Identifier, contactItem1.Section());
    ASSERT_EQ(ot::contact::ContactItemType::Employee, contactItem1.Type());
    ASSERT_EQ("testValue", contactItem1.Value());
    ASSERT_EQ(contactItem1.Start(), NULL_START);
    ASSERT_EQ(contactItem1.End(), NULL_END);

    ASSERT_TRUE(contactItem1.isActive());
    ASSERT_FALSE(contactItem1.isLocal());
    ASSERT_FALSE(contactItem1.isPrimary());
}

TEST_F(Test_ContactItem, copy_constructor)
{
    ot::ContactItem copiedContactItem(contactItem_);

    ASSERT_EQ(contactItem_.ID(), copiedContactItem.ID());
    ASSERT_EQ(contactItem_.Version(), copiedContactItem.Version());
    ASSERT_EQ(contactItem_.Section(), copiedContactItem.Section());
    ASSERT_EQ(contactItem_.Type(), copiedContactItem.Type());
    ASSERT_EQ(contactItem_.Value(), copiedContactItem.Value());
    ASSERT_EQ(contactItem_.Start(), copiedContactItem.Start());
    ASSERT_EQ(contactItem_.End(), copiedContactItem.End());

    ASSERT_EQ(contactItem_.isActive(), copiedContactItem.isActive());
    ASSERT_EQ(contactItem_.isLocal(), copiedContactItem.isLocal());
    ASSERT_EQ(contactItem_.isPrimary(), copiedContactItem.isPrimary());
}

TEST_F(Test_ContactItem, operator_equal_true)
{
    ASSERT_EQ(contactItem_, contactItem_);
}

TEST_F(Test_ContactItem, operator_equal_false)
{
    ot::ContactItem contactItem2(
        api_,
        std::string("testNym2"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::Employee,
        std::string("testValue2"),
        {ot::contact::ContactItemAttribute::Active},
        NULL_START,
        NULL_END,
        "");

    // Can't use ASSERT_NE because there's no != operator defined for
    // ContactItem.
    ASSERT_FALSE(contactItem_ == contactItem2);
}

TEST_F(Test_ContactItem, operator_proto_equal)
{
    ot::proto::ContactItem protoItem = contactItem_;

    ASSERT_EQ(contactItem_.ID().str(), protoItem.id());
    ASSERT_EQ(contactItem_.Value(), protoItem.value());
    ASSERT_EQ(contactItem_.Version(), protoItem.version());
    ASSERT_EQ(
        contactItem_.Type(),
        ot::contact::internal::translate(protoItem.type()));
    ASSERT_EQ(contactItem_.Start(), protoItem.start());
    ASSERT_EQ(contactItem_.End(), protoItem.end());
}

TEST_F(Test_ContactItem, operator_proto_not_equal)
{
    ot::proto::ContactItem protoItem = contactItem_;
    ot::proto::ContactItem protoItemNoId = contactItem_.Serialize();

    ASSERT_NE(protoItemNoId.id(), protoItem.id());
}

TEST_F(Test_ContactItem, public_accessors)
{
    const ot::OTIdentifier identifier(
        ot::Identifier::Factory(ot::identity::credential::Contact::ClaimID(
            api_,
            "testNym",
            ot::contact::ContactSectionName::Identifier,
            ot::contact::ContactItemType::Employee,
            NULL_START,
            NULL_END,
            "testValue",
            "")));
    ASSERT_EQ(identifier, contactItem_.ID());
    ASSERT_EQ(
        ot::contact::ContactSectionName::Identifier, contactItem_.Section());
    ASSERT_EQ(ot::contact::ContactItemType::Employee, contactItem_.Type());
    ASSERT_EQ("testValue", contactItem_.Value());
    ASSERT_EQ(contactItem_.Start(), NULL_START);
    ASSERT_EQ(contactItem_.End(), NULL_END);
    ASSERT_EQ(CONTACT_CONTACT_DATA_VERSION, contactItem_.Version());

    ASSERT_TRUE(contactItem_.isActive());
    ASSERT_FALSE(contactItem_.isLocal());
    ASSERT_FALSE(contactItem_.isPrimary());
}

TEST_F(Test_ContactItem, public_setters)
{
    const auto now = std::time(nullptr);

    const auto& valueItem = contactItem_.SetValue("newTestValue");
    ASSERT_FALSE(valueItem == contactItem_);
    ASSERT_STREQ(valueItem.Value().c_str(), "newTestValue");

    const auto& startItem = contactItem_.SetStart(now);
    ASSERT_FALSE(startItem == contactItem_);
    ASSERT_EQ(now, startItem.Start());
    ASSERT_NE(startItem.Start(), NULL_START);

    const auto& endItem = contactItem_.SetEnd(now);
    ASSERT_FALSE(endItem == contactItem_);
    ASSERT_EQ(now, endItem.End());
    ASSERT_NE(NULL_END, endItem.End());

    // _contactItem is active, so test setting active to false first.
    const auto& notActiveItem = contactItem_.SetActive(false);
    ASSERT_FALSE(notActiveItem == contactItem_);
    ASSERT_FALSE(notActiveItem.isActive());
    const auto& activeItem = notActiveItem.SetActive(true);
    ASSERT_FALSE(activeItem == notActiveItem);
    ASSERT_TRUE(activeItem.isActive());

    const auto& localItem = contactItem_.SetLocal(true);
    ASSERT_FALSE(localItem == contactItem_);
    ASSERT_TRUE(localItem.isLocal());
    const auto& notLocalItem = localItem.SetLocal(false);
    ASSERT_FALSE(notLocalItem == localItem);
    ASSERT_FALSE(notLocalItem.isLocal());

    // First, create an item with no attributes.
    const auto& notPrimaryItem = contactItem_.SetActive(false);
    ASSERT_FALSE(notPrimaryItem == contactItem_);
    ASSERT_FALSE(notPrimaryItem.isPrimary());
    ASSERT_FALSE(notPrimaryItem.isActive());
    ASSERT_FALSE(notPrimaryItem.isLocal());
    // Now, set the primary attribute, and test for primary and active.
    const auto& primaryItem = notPrimaryItem.SetPrimary(true);
    ASSERT_FALSE(primaryItem == notPrimaryItem);
    ASSERT_TRUE(primaryItem.isPrimary());
    ASSERT_TRUE(primaryItem.isActive());
}

TEST_F(Test_ContactItem, Serialize)
{
    // Test without id.
    ot::proto::ContactItem protoItem = contactItem_.Serialize();

    ASSERT_EQ(contactItem_.Value(), protoItem.value());
    ASSERT_EQ(contactItem_.Version(), protoItem.version());
    ASSERT_EQ(
        contactItem_.Type(),
        ot::contact::internal::translate(protoItem.type()));
    ASSERT_EQ(contactItem_.Start(), protoItem.start());
    ASSERT_EQ(contactItem_.End(), protoItem.end());

    // Test with id.
    protoItem = contactItem_.Serialize(true);

    ASSERT_EQ(contactItem_.ID().str(), protoItem.id());
    ASSERT_EQ(contactItem_.Value(), protoItem.value());
    ASSERT_EQ(contactItem_.Version(), protoItem.version());
    ASSERT_EQ(
        contactItem_.Type(),
        ot::contact::internal::translate(protoItem.type()));
    ASSERT_EQ(contactItem_.Start(), protoItem.start());
    ASSERT_EQ(contactItem_.End(), protoItem.end());
}
