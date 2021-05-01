// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <iterator>
#include <map>
#include <memory>
#include <string>

#include "1_Internal.hpp"
#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "internal/api/client/Client.hpp"
#include "internal/contact/Contact.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/contact/ContactGroup.hpp"
#include "opentxs/contact/ContactItem.hpp"
#include "opentxs/contact/ContactItemAttribute.hpp"
#include "opentxs/contact/ContactSection.hpp"
#include "opentxs/contact/ContactSectionName.hpp"
#include "opentxs/core/Identifier.hpp"

namespace
{

class Test_ContactSection : public ::testing::Test
{
public:
    Test_ContactSection()
        : api_(dynamic_cast<const ot::api::client::internal::Manager&>(
              ot::Context().StartClient({}, 0)))
        , contactSection_(
              api_,
              std::string("testContactSectionNym1"),
              CONTACT_CONTACT_DATA_VERSION,
              CONTACT_CONTACT_DATA_VERSION,
              ot::contact::ContactSectionName::Identifier,
              ot::ContactSection::GroupMap{})
        , contactGroup_(new ot::ContactGroup(
              std::string("testContactGroupNym1"),
              ot::contact::ContactSectionName::Identifier,
              ot::contact::ContactItemType::Employee,
              {}))
        , activeContactItem_(new ot::ContactItem(
              api_,
              std::string("activeContactItem"),
              CONTACT_CONTACT_DATA_VERSION,
              CONTACT_CONTACT_DATA_VERSION,
              ot::contact::ContactSectionName::Identifier,
              ot::contact::ContactItemType::Employee,
              std::string("activeContactItemValue"),
              {ot::contact::ContactItemAttribute::Active},
              NULL_START,
              NULL_END,
              ""))
    {
    }

    const ot::api::client::internal::Manager& api_;
    const ot::ContactSection contactSection_;
    const std::shared_ptr<ot::ContactGroup> contactGroup_;
    const std::shared_ptr<ot::ContactItem> activeContactItem_;
};

}  // namespace

TEST_F(Test_ContactSection, first_constructor)
{
    const auto& group1 = std::shared_ptr<ot::ContactGroup>(
        new ot::ContactGroup(contactGroup_->AddItem(activeContactItem_)));
    ot::ContactSection::GroupMap groupMap{
        {ot::contact::ContactItemType::Employee, group1}};

    const ot::ContactSection section1(
        api_,
        "testContactSectionNym1",
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        groupMap);
    ASSERT_EQ(ot::contact::ContactSectionName::Identifier, section1.Type());
    ASSERT_EQ(CONTACT_CONTACT_DATA_VERSION, section1.Version());
    ASSERT_EQ(section1.Size(), 1);
    ASSERT_EQ(
        group1->Size(),
        section1.Group(ot::contact::ContactItemType::Employee)->Size());
    ASSERT_NE(section1.end(), section1.begin());
}

TEST_F(Test_ContactSection, first_constructor_different_versions)
{
    // Test private static method check_version.
    const ot::ContactSection section2(
        api_,
        "testContactSectionNym2",
        CONTACT_CONTACT_DATA_VERSION - 1,  // previous version
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::ContactSection::GroupMap{});
    ASSERT_EQ(CONTACT_CONTACT_DATA_VERSION, section2.Version());
}

TEST_F(Test_ContactSection, second_constructor)
{
    const ot::ContactSection section1(
        api_,
        "testContactSectionNym1",
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        activeContactItem_);
    ASSERT_EQ(ot::contact::ContactSectionName::Identifier, section1.Type());
    ASSERT_EQ(CONTACT_CONTACT_DATA_VERSION, section1.Version());
    ASSERT_EQ(section1.Size(), 1);
    ASSERT_NE(nullptr, section1.Group(ot::contact::ContactItemType::Employee));
    ASSERT_EQ(
        section1.Group(ot::contact::ContactItemType::Employee)->Size(), 1);
    ASSERT_NE(section1.end(), section1.begin());
}

TEST_F(Test_ContactSection, copy_constructor)
{
    const auto& section1(contactSection_.AddItem(activeContactItem_));

    ot::ContactSection copiedContactSection(section1);
    ASSERT_EQ(section1.Type(), copiedContactSection.Type());
    ASSERT_EQ(section1.Version(), copiedContactSection.Version());
    ASSERT_EQ(copiedContactSection.Size(), 1);
    ASSERT_NE(
        nullptr,
        copiedContactSection.Group(ot::contact::ContactItemType::Employee));
    ASSERT_EQ(
        copiedContactSection.Group(ot::contact::ContactItemType::Employee)
            ->Size(),
        1);
    ASSERT_NE(copiedContactSection.end(), copiedContactSection.begin());
}

TEST_F(Test_ContactSection, operator_plus)
{
    // Combine two sections with one item each of the same type.
    const auto& section1 = contactSection_.AddItem(activeContactItem_);

    const std::shared_ptr<ot::ContactItem> contactItem2(new ot::ContactItem(
        api_,
        std::string("activeContactItem2"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::Employee,
        std::string("activeContactItemValue2"),
        {ot::contact::ContactItemAttribute::Active},
        NULL_START,
        NULL_END,
        ""));
    const ot::ContactSection section2(
        api_,
        "testContactSectionNym2",
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        contactItem2);

    const auto& section3 = section1 + section2;
    // Verify the section has one group.
    ASSERT_EQ(section3.Size(), 1);
    // Verify the group has two items.
    ASSERT_EQ(
        section3.Group(ot::contact::ContactItemType::Employee)->Size(), 2);

    // Add a section that has one group with one item of the same type, and
    // another group with one item of a different type.
    const std::shared_ptr<ot::ContactItem> contactItem3(new ot::ContactItem(
        api_,
        std::string("activeContactItem3"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::Employee,
        std::string("activeContactItemValue3"),
        {ot::contact::ContactItemAttribute::Active},
        NULL_START,
        NULL_END,
        ""));
    const std::shared_ptr<ot::ContactItem> contactItem4(new ot::ContactItem(
        api_,
        std::string("activeContactItem4"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::SSL,
        std::string("activeContactItemValue4"),
        {ot::contact::ContactItemAttribute::Active},
        NULL_START,
        NULL_END,
        ""));
    const ot::ContactSection section4(
        api_,
        "testContactSectionNym4",
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        contactItem3);
    const auto& section5 = section4.AddItem(contactItem4);

    const auto& section6 = section3 + section5;
    // Verify the section has two groups.
    ASSERT_EQ(section6.Size(), 2);
    // Verify the first group has three items.
    ASSERT_EQ(
        section6.Group(ot::contact::ContactItemType::Employee)->Size(), 3);
    // Verify the second group has one item.
    ASSERT_EQ(section6.Group(ot::contact::ContactItemType::SSL)->Size(), 1);
}

TEST_F(Test_ContactSection, operator_plus_different_versions)
{
    // rhs version less than lhs
    const ot::ContactSection section2(
        api_,
        "testContactSectionNym2",
        CONTACT_CONTACT_DATA_VERSION - 1,
        CONTACT_CONTACT_DATA_VERSION - 1,
        ot::contact::ContactSectionName::Identifier,
        ot::ContactSection::GroupMap{});

    const auto& section3 = contactSection_ + section2;
    // Verify the new section has the latest version.
    ASSERT_EQ(CONTACT_CONTACT_DATA_VERSION, section3.Version());

    // lhs version less than rhs
    const auto& section4 = section2 + contactSection_;
    // Verify the new section has the latest version.
    ASSERT_EQ(CONTACT_CONTACT_DATA_VERSION, section4.Version());
}

TEST_F(Test_ContactSection, AddItem)
{
    // Add an item to a SCOPE section.
    const std::shared_ptr<ot::ContactItem> scopeContactItem(new ot::ContactItem(
        api_,
        std::string("scopeContactItem"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Scope,
        ot::contact::ContactItemType::Individual,
        std::string("scopeContactItemValue"),
        {ot::contact::ContactItemAttribute::Local},
        NULL_START,
        NULL_END,
        ""));
    const ot::ContactSection section1(
        api_,
        "testContactSectionNym2",
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Scope,
        ot::ContactSection::GroupMap{});
    const auto& section2 = section1.AddItem(scopeContactItem);
    ASSERT_EQ(section2.Size(), 1);
    ASSERT_EQ(
        section2.Group(ot::contact::ContactItemType::Individual)->Size(), 1);
    ASSERT_TRUE(section2.Claim(scopeContactItem->ID())->isPrimary());
    ASSERT_TRUE(section2.Claim(scopeContactItem->ID())->isActive());

    // Add an item to a non-scope section.
    const auto& section4 = contactSection_.AddItem(activeContactItem_);
    ASSERT_EQ(section4.Size(), 1);
    ASSERT_EQ(
        section4.Group(ot::contact::ContactItemType::Employee)->Size(), 1);
    // Add a second item of the same type.
    const std::shared_ptr<ot::ContactItem> contactItem2(new ot::ContactItem(
        api_,
        std::string("activeContactItem2"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::Employee,
        std::string("activeContactItemValue2"),
        {ot::contact::ContactItemAttribute::Active},
        NULL_START,
        NULL_END,
        ""));
    const auto& section5 = section4.AddItem(contactItem2);
    // Verify there are two items.
    ASSERT_EQ(section5.Size(), 1);
    ASSERT_EQ(
        section5.Group(ot::contact::ContactItemType::Employee)->Size(), 2);

    // Add an item of a different type.
    const std::shared_ptr<ot::ContactItem> contactItem3(new ot::ContactItem(
        api_,
        std::string("activeContactItem3"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::SSL,
        std::string("activeContactItemValue3"),
        {ot::contact::ContactItemAttribute::Active},
        NULL_START,
        NULL_END,
        ""));
    const auto& section6 = section5.AddItem(contactItem3);
    // Verify there are two groups.
    ASSERT_EQ(section6.Size(), 2);
    // Verify there is one in the second group.
    ASSERT_EQ(section6.Group(ot::contact::ContactItemType::SSL)->Size(), 1);
}

TEST_F(Test_ContactSection, AddItem_different_versions)
{
    // Add an item with a newer version to a SCOPE section.
    const std::shared_ptr<ot::ContactItem> scopeContactItem(new ot::ContactItem(
        api_,
        std::string("scopeContactItem"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Scope,
        ot::contact::ContactItemType::BOT,
        std::string("scopeContactItemValue"),
        {ot::contact::ContactItemAttribute::Local},
        NULL_START,
        NULL_END,
        ""));
    const ot::ContactSection section1(
        api_,
        "testContactSectionNym2",
        3,  // version of CONTACTSECTION_SCOPE section before CITEMTYPE_BOT was
            // added
        3,
        ot::contact::ContactSectionName::Scope,
        ot::ContactSection::GroupMap{});
    const auto& section2 = section1.AddItem(scopeContactItem);
    ASSERT_EQ(section2.Size(), 1);
    ASSERT_EQ(section2.Group(ot::contact::ContactItemType::BOT)->Size(), 1);
    ASSERT_TRUE(section2.Claim(scopeContactItem->ID())->isPrimary());
    ASSERT_TRUE(section2.Claim(scopeContactItem->ID())->isActive());
    // Verify the section version has been updated to the minimum version to
    // support CITEMTYPE_BOT.
    ASSERT_EQ(section2.Version(), 4);

    // Add an item with a newer version to a non-scope section.
    const std::shared_ptr<ot::ContactItem> contactItem2(new ot::ContactItem(
        api_,
        std::string("contactItem2"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Relationship,
        ot::contact::ContactItemType::Owner,
        std::string("contactItem2Value"),
        {ot::contact::ContactItemAttribute::Local},
        NULL_START,
        NULL_END,
        ""));
    const ot::ContactSection section3(
        api_,
        "testContactSectionNym3",
        3,  // version of CONTACTSECTION_RELATIONSHIP section before
            // CITEMTYPE_OWNER was added
        3,
        ot::contact::ContactSectionName::Relationship,
        ot::ContactSection::GroupMap{});
    const auto& section4 = section3.AddItem(contactItem2);
    ASSERT_EQ(section4.Size(), 1);
    ASSERT_EQ(section4.Group(ot::contact::ContactItemType::Owner)->Size(), 1);
    // Verify the section version has been updated to the minimum version to
    // support CITEMTYPE_OWNER.
    ASSERT_EQ(4, section4.Version());
}

TEST_F(Test_ContactSection, begin)
{
    ot::ContactSection::GroupMap::const_iterator it = contactSection_.begin();
    ASSERT_EQ(contactSection_.end(), it);
    ASSERT_EQ(std::distance(it, contactSection_.end()), 0);

    const auto& section1 = contactSection_.AddItem(activeContactItem_);
    it = section1.begin();
    ASSERT_NE(section1.end(), it);
    ASSERT_EQ(std::distance(it, section1.end()), 1);

    std::advance(it, 1);
    ASSERT_EQ(section1.end(), it);
    ASSERT_EQ(std::distance(it, section1.end()), 0);
}

TEST_F(Test_ContactSection, Claim_found)
{
    const auto& section1 = contactSection_.AddItem(activeContactItem_);

    const std::shared_ptr<ot::ContactItem>& claim =
        section1.Claim(activeContactItem_->ID());
    ASSERT_NE(nullptr, claim);
    ASSERT_EQ(activeContactItem_->ID(), claim->ID());

    // Find a claim in a different group.
    const std::shared_ptr<ot::ContactItem> contactItem2(new ot::ContactItem(
        api_,
        std::string("activeContactItem2"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::SSL,
        std::string("activeContactItemValue2"),
        {ot::contact::ContactItemAttribute::Active},
        NULL_START,
        NULL_END,
        ""));
    const auto& section2 = section1.AddItem(contactItem2);
    const std::shared_ptr<ot::ContactItem>& claim2 =
        section2.Claim(contactItem2->ID());
    ASSERT_NE(nullptr, claim2);
    ASSERT_EQ(contactItem2->ID(), claim2->ID());
}

TEST_F(Test_ContactSection, Claim_notfound)
{
    const std::shared_ptr<ot::ContactItem>& claim =
        contactSection_.Claim(activeContactItem_->ID());
    ASSERT_FALSE(claim);
}

TEST_F(Test_ContactSection, end)
{
    ot::ContactSection::GroupMap::const_iterator it = contactSection_.end();
    ASSERT_EQ(contactSection_.begin(), it);
    ASSERT_EQ(std::distance(contactSection_.begin(), it), 0);

    const auto& section1 = contactSection_.AddItem(activeContactItem_);
    it = section1.end();
    ASSERT_NE(section1.begin(), it);
    ASSERT_EQ(std::distance(section1.begin(), it), 1);

    std::advance(it, -1);
    ASSERT_EQ(section1.begin(), it);
    ASSERT_EQ(std::distance(section1.begin(), it), 0);
}

TEST_F(Test_ContactSection, Group_found)
{
    const auto& section1 = contactSection_.AddItem(activeContactItem_);
    ASSERT_NE(nullptr, section1.Group(ot::contact::ContactItemType::Employee));
}

TEST_F(Test_ContactSection, Group_notfound)
{
    ASSERT_FALSE(contactSection_.Group(ot::contact::ContactItemType::Employee));
}

TEST_F(Test_ContactSection, HaveClaim_true)
{
    const auto& section1 = contactSection_.AddItem(activeContactItem_);

    ASSERT_TRUE(section1.HaveClaim(activeContactItem_->ID()));

    // Find a claim in a different group.
    const std::shared_ptr<ot::ContactItem> contactItem2(new ot::ContactItem(
        api_,
        std::string("activeContactItem2"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::SSL,
        std::string("activeContactItemValue2"),
        {ot::contact::ContactItemAttribute::Active},
        NULL_START,
        NULL_END,
        ""));
    const auto& section2 = section1.AddItem(contactItem2);
    ASSERT_TRUE(section2.HaveClaim(contactItem2->ID()));
}

TEST_F(Test_ContactSection, HaveClaim_false)
{
    ASSERT_FALSE(contactSection_.HaveClaim(activeContactItem_->ID()));
}

TEST_F(Test_ContactSection, Delete)
{
    const auto& section1 = contactSection_.AddItem(activeContactItem_);
    ASSERT_TRUE(section1.HaveClaim(activeContactItem_->ID()));

    // Add a second item to help testing the size after trying to delete twice.
    const std::shared_ptr<ot::ContactItem> contactItem2(new ot::ContactItem(
        api_,
        std::string("activeContactItem2"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::Employee,
        std::string("activeContactItemValue2"),
        {ot::contact::ContactItemAttribute::Active},
        NULL_START,
        NULL_END,
        ""));
    const auto& section2 = section1.AddItem(contactItem2);
    ASSERT_EQ(
        section2.Group(ot::contact::ContactItemType::Employee)->Size(), 2);

    const auto& section3 = section2.Delete(activeContactItem_->ID());
    // Verify the item was deleted.
    ASSERT_FALSE(section3.HaveClaim(activeContactItem_->ID()));
    ASSERT_EQ(
        section3.Group(ot::contact::ContactItemType::Employee)->Size(), 1);

    const auto& section4 = section3.Delete(activeContactItem_->ID());
    // Verify trying to delete the item again didn't change anything.
    ASSERT_EQ(
        section4.Group(ot::contact::ContactItemType::Employee)->Size(), 1);

    // Add an item of a different type.
    const std::shared_ptr<ot::ContactItem> contactItem3(new ot::ContactItem(
        api_,
        std::string("activeContactItem3"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::SSL,
        std::string("activeContactItemValue3"),
        {ot::contact::ContactItemAttribute::Active},
        NULL_START,
        NULL_END,
        ""));
    const auto& section5 = section4.AddItem(contactItem3);
    // Verify the section has two groups.
    ASSERT_EQ(section5.Size(), 2);
    ASSERT_EQ(section5.Group(ot::contact::ContactItemType::SSL)->Size(), 1);
    ASSERT_TRUE(section5.HaveClaim(contactItem3->ID()));

    const auto& section6 = section5.Delete(contactItem3->ID());
    // Verify the item was deleted and the group was removed.
    ASSERT_EQ(section6.Size(), 1);
    ASSERT_FALSE(section6.HaveClaim(contactItem3->ID()));
    ASSERT_FALSE(section6.Group(ot::contact::ContactItemType::SSL));
}

TEST_F(Test_ContactSection, SerializeTo)
{
    // Serialize without ids.
    const auto& section1 = contactSection_.AddItem(activeContactItem_);
    auto bytes = ot::Space{};
    ASSERT_TRUE(section1.Serialize(ot::writer(bytes), false));

    auto restored1 =
        ot::ContactSection{api_, "ContactDataNym1", section1.Version(), bytes};

    ASSERT_EQ(restored1.Size(), section1.Size());
    ASSERT_EQ(restored1.Type(), section1.Type());
    auto group_iterator = restored1.begin();
    ASSERT_EQ(group_iterator->first, ot::contact::ContactItemType::Employee);
    auto group1 = group_iterator->second;
    ASSERT_TRUE(group1);
    auto item_iterator = group1->begin();
    auto contact_item = item_iterator->second;
    ASSERT_TRUE(contact_item);
    ASSERT_EQ(activeContactItem_->Value(), contact_item->Value());
    ASSERT_EQ(activeContactItem_->Version(), contact_item->Version());
    ASSERT_EQ(activeContactItem_->Type(), contact_item->Type());
    ASSERT_EQ(activeContactItem_->Start(), contact_item->Start());
    ASSERT_EQ(activeContactItem_->End(), contact_item->End());

    //    // Serialize with ids.
    auto restored2 =
        ot::ContactSection{api_, "ContactDataNym1", section1.Version(), bytes};

    ASSERT_EQ(restored2.Size(), section1.Size());
    ASSERT_EQ(restored2.Type(), section1.Type());
    group_iterator = restored2.begin();
    ASSERT_EQ(group_iterator->first, ot::contact::ContactItemType::Employee);
    group1 = group_iterator->second;
    ASSERT_TRUE(group1);
    item_iterator = group1->begin();
    contact_item = item_iterator->second;
    ASSERT_TRUE(contact_item);
    ASSERT_EQ(activeContactItem_->Value(), contact_item->Value());
    ASSERT_EQ(activeContactItem_->Version(), contact_item->Version());
    ASSERT_EQ(activeContactItem_->Type(), contact_item->Type());
    ASSERT_EQ(activeContactItem_->Start(), contact_item->Start());
    ASSERT_EQ(activeContactItem_->End(), contact_item->End());
}

TEST_F(Test_ContactSection, Size)
{
    ASSERT_EQ(contactSection_.Size(), 0);
    const auto& section1 = contactSection_.AddItem(activeContactItem_);
    ASSERT_EQ(section1.Size(), 1);

    // Add a second item of the same type.
    const std::shared_ptr<ot::ContactItem> contactItem2(new ot::ContactItem(
        api_,
        std::string("activeContactItem2"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::Employee,
        std::string("activeContactItemValue2"),
        {ot::contact::ContactItemAttribute::Active},
        NULL_START,
        NULL_END,
        ""));
    const auto& section2 = section1.AddItem(contactItem2);
    // Verify the size is the same.
    ASSERT_EQ(section2.Size(), 1);

    // Add an item of a different type.
    const std::shared_ptr<ot::ContactItem> contactItem3(new ot::ContactItem(
        api_,
        std::string("activeContactItem3"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::SSL,
        std::string("activeContactItemValue3"),
        {ot::contact::ContactItemAttribute::Active},
        NULL_START,
        NULL_END,
        ""));
    const auto& section3 = section2.AddItem(contactItem3);
    // Verify the size is now two.
    ASSERT_EQ(section3.Size(), 2);

    // Delete an item from the first group.
    const auto& section4 = section3.Delete(contactItem2->ID());
    // Verify the size is still two.
    ASSERT_EQ(section4.Size(), 2);

    // Delete the item from the second group.
    const auto& section5 = section4.Delete(contactItem3->ID());
    // Verify that the size is now one.
    ASSERT_EQ(section5.Size(), 1);
}

TEST_F(Test_ContactSection, Type)
{
    ASSERT_EQ(
        ot::contact::ContactSectionName::Identifier, contactSection_.Type());
}

TEST_F(Test_ContactSection, Version)
{
    ASSERT_EQ(CONTACT_CONTACT_DATA_VERSION, contactSection_.Version());
}
