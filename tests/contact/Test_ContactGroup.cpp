// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <utility>

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
#include "opentxs/contact/ContactSectionName.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/protobuf/ContactItem.pb.h"
#include "opentxs/protobuf/ContactSection.pb.h"

namespace
{

class Test_ContactGroup : public ::testing::Test
{
public:
    Test_ContactGroup()
        : api_(dynamic_cast<const ot::api::client::internal::Manager&>(
              ot::Context().StartClient({}, 0)))
        , contactGroup_(
              std::string("testContactGroupNym1"),
              ot::contact::ContactSectionName::Identifier,
              ot::contact::ContactItemType::Employee,
              {})
        , primary_(new ot::ContactItem(
              api_,
              std::string("primaryContactItem"),
              CONTACT_CONTACT_DATA_VERSION,
              CONTACT_CONTACT_DATA_VERSION,
              ot::contact::ContactSectionName::Identifier,
              ot::contact::ContactItemType::Employee,
              std::string("primaryContactItemValue"),
              {ot::contact::ContactItemAttribute::Primary},
              NULL_START,
              NULL_END,
              ""))
        , active_(new ot::ContactItem(
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
    const ot::ContactGroup contactGroup_;
    const std::shared_ptr<ot::ContactItem> primary_;
    const std::shared_ptr<ot::ContactItem> active_;
};

}  // namespace

TEST_F(Test_ContactGroup, first_constructor)
{
    // Test constructing a group with a map containing two primary items.
    const std::shared_ptr<ot::ContactItem> primary2(new ot::ContactItem(
        api_,
        std::string("primaryContactItemNym2"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::Employee,
        std::string("primaryContactItemValue2"),
        {ot::contact::ContactItemAttribute::Primary},
        NULL_START,
        NULL_END,
        ""));

    ot::ContactGroup::ItemMap map;
    map[primary_->ID()] = primary_;
    map[primary2->ID()] = primary2;

    const ot::ContactGroup group1(
        std::string("testContactGroupNym1"),
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::Employee,
        map);
    // Verify two items were added.
    ASSERT_EQ(2, group1.Size());
    ASSERT_EQ(ot::contact::ContactItemType::Employee, group1.Type());
    // Verify only one item is primary.
    if (primary_->ID() == group1.Primary()) {
        ASSERT_TRUE(group1.Claim(primary_->ID())->isPrimary());
        ASSERT_FALSE(group1.Claim(primary2->ID())->isPrimary());
    } else {
        ASSERT_FALSE(group1.Claim(primary_->ID())->isPrimary());
        ASSERT_TRUE(group1.Claim(primary2->ID())->isPrimary());
    }
}

TEST_F(Test_ContactGroup, first_constructor_no_items)
{
    // Test constructing a group with a map containing no items.
    const ot::ContactGroup group1(
        std::string("testContactGroupNym1"),
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::Employee,
        {});
    // Verify the private static methods didn't blow up.
    ASSERT_EQ(group1.Size(), 0);
    ASSERT_EQ(ot::contact::ContactItemType::Employee, group1.Type());
}

TEST_F(Test_ContactGroup, second_constructor)
{
    const ot::ContactGroup group1(
        std::string("testContactGroupNym1"),
        ot::contact::ContactSectionName::Identifier,
        active_);

    ASSERT_EQ(1, group1.Size());
    // Verify the group type matches the type of the item.
    ASSERT_EQ(active_->Type(), group1.Type());
    ASSERT_EQ(active_->ID(), group1.begin()->second->ID());
}

TEST_F(Test_ContactGroup, copy_constructor)
{
    const ot::ContactGroup group1(
        std::string("testContactGroupNym1"),
        ot::contact::ContactSectionName::Identifier,
        active_);

    ot::ContactGroup copiedContactGroup(group1);

    ASSERT_EQ(1, copiedContactGroup.Size());
    // Verify the group type matches the type of the item.
    ASSERT_EQ(active_->Type(), copiedContactGroup.Type());
    ASSERT_EQ(active_->ID(), copiedContactGroup.begin()->second->ID());
}

TEST_F(Test_ContactGroup, move_constructor)
{
    ot::ContactGroup movedContactGroup(contactGroup_.AddItem(active_));

    ASSERT_EQ(1, movedContactGroup.Size());
    // Verify the group type matches the type of the item.
    ASSERT_EQ(active_->Type(), movedContactGroup.Type());
    ASSERT_EQ(active_->ID(), movedContactGroup.begin()->second->ID());
}

TEST_F(Test_ContactGroup, operator_plus)
{
    // Test adding a group with a primary (rhs) to a group without a primary.
    const auto& group1 = contactGroup_.AddItem(active_);
    const auto& group2 = contactGroup_.AddItem(primary_);
    const auto& group3 = group1 + group2;
    ASSERT_EQ(2, group3.Size());
    // Verify that the primary for the new group comes from rhs.
    ASSERT_EQ(group2.Primary(), group3.Primary());

    // Test adding a group with 2 items to a group with 1 item.
    const std::shared_ptr<ot::ContactItem> primary2(new ot::ContactItem(
        api_,
        std::string("primaryContactItemNym2"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::Employee,
        std::string("primaryContactItemValue2"),
        {ot::contact::ContactItemAttribute::Primary},
        NULL_START,
        NULL_END,
        ""));
    const auto& group4 = contactGroup_.AddItem(primary2);
    const auto& group5 = contactGroup_.AddItem(primary_);
    const auto& group6 = group5.AddItem(active_);

    const auto& group7 = group4 + group6;
    // Verify that the group has 3 items.
    ASSERT_EQ(3, group7.Size());
    // Verify that the primary of the new group came from the lhs.
    ASSERT_EQ(group4.Primary(), group7.Primary());

    for (auto it(group7.begin()); it != group7.end(); ++it) {
        if (it->second->ID() == primary_->ID()) {
            // Verify that the item that was primary on the rhs doesn't have
            // the primary attribute.
            ASSERT_FALSE(it->second->isPrimary());
        }
    }
}

TEST_F(Test_ContactGroup, AddItem)
{
    const auto& group1 = contactGroup_.AddItem(active_);
    ASSERT_EQ(1, group1.Size());
    ASSERT_EQ(active_->ID(), group1.begin()->second->ID());

    // Test whether AddItem handles items that have already been added.
    const auto& group2 = group1.AddItem(active_);
    // Verify that there is still only one item.
    ASSERT_EQ(1, group2.Size());

    // Test that AddItem handles adding a primary.
    const auto& group3 = contactGroup_.AddItem(primary_);
    ASSERT_EQ(1, group3.Size());
    ASSERT_EQ(primary_->ID(), group3.Primary());
    ASSERT_TRUE(group3.begin()->second->isPrimary());
}

TEST_F(Test_ContactGroup, AddPrimary)
{
    // Test that AddPrimary ignores a null pointer.
    const auto& group1 = contactGroup_.AddPrimary(nullptr);
    ASSERT_TRUE(group1.Primary().empty());

    // Test that AddPrimary sets the primary attribute on an active item.
    const auto& group2 = contactGroup_.AddPrimary(active_);
    // Verify that primary is set to the item id.
    ASSERT_EQ(active_->ID(), group2.Primary());
    // Verify that the item has the primary attribute.
    ASSERT_TRUE(group2.begin()->second->isPrimary());

    // Test adding a primary when the group already has one.
    const auto& group3 = contactGroup_.AddPrimary(primary_);
    const auto& group4 = group3.AddPrimary(active_);
    // Verify that primary is set to the new item id.
    ASSERT_EQ(active_->ID(), group4.Primary());

    for (auto it(group4.begin()); it != group4.end(); ++it) {
        if (it->second->ID() == primary_->ID()) {
            // Verify that the old primary item doesn't have the primary
            // attribute.
            ASSERT_FALSE(it->second->isPrimary());
        } else if (it->second->ID() == active_->ID()) {
            // Verify that the new primary item has the primary attribute.
            ASSERT_TRUE(it->second->isPrimary());
        }
    }
}

TEST_F(Test_ContactGroup, begin)
{
    ot::ContactGroup::ItemMap::const_iterator it = contactGroup_.begin();
    ASSERT_EQ(contactGroup_.end(), it);
    ASSERT_EQ(std::distance(it, contactGroup_.end()), 0);

    const auto& group1 = contactGroup_.AddItem(active_);
    it = group1.begin();
    ASSERT_NE(group1.end(), it);
    ASSERT_EQ(1, std::distance(it, group1.end()));

    std::advance(it, 1);
    ASSERT_EQ(group1.end(), it);
    ASSERT_EQ(std::distance(it, group1.end()), 0);
}

TEST_F(Test_ContactGroup, Best_none) { ASSERT_FALSE(contactGroup_.Best()); }

TEST_F(Test_ContactGroup, Best_primary)
{
    const auto& group1 = contactGroup_.AddItem(primary_);

    const std::shared_ptr<ot::ContactItem>& best = group1.Best();
    ASSERT_NE(nullptr, best);
    ASSERT_EQ(primary_->ID(), best->ID());
}

TEST_F(Test_ContactGroup, Best_active_and_local)
{
    const std::shared_ptr<ot::ContactItem> local(new ot::ContactItem(
        api_,
        std::string("localContactItemNym"),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        ot::contact::ContactSectionName::Identifier,
        ot::contact::ContactItemType::Employee,
        std::string("localContactItemValue"),
        {ot::contact::ContactItemAttribute::Local},
        NULL_START,
        NULL_END,
        ""));
    const auto& group1 = contactGroup_.AddItem(active_);
    const auto& group2 = group1.AddItem(local);

    const std::shared_ptr<ot::ContactItem>& best = group2.Best();
    // Verify the best item is the active one.
    ASSERT_NE(nullptr, best);
    ASSERT_EQ(active_->ID(), best->ID());
    ASSERT_TRUE(best->isActive());

    const auto& group3 = group2.Delete(active_->ID());
    const std::shared_ptr<ot::ContactItem>& best2 = group3.Best();
    // Verify the best item is the local one.
    ASSERT_NE(nullptr, best2);
    ASSERT_EQ(local->ID(), best2->ID());
    ASSERT_TRUE(best2->isLocal());
}

TEST_F(Test_ContactGroup, Claim_found)
{
    const auto& group1 = contactGroup_.AddItem(active_);

    const std::shared_ptr<ot::ContactItem>& claim = group1.Claim(active_->ID());
    ASSERT_NE(nullptr, claim);
    ASSERT_EQ(active_->ID(), claim->ID());
}

TEST_F(Test_ContactGroup, Claim_notfound)
{
    const std::shared_ptr<ot::ContactItem>& claim =
        contactGroup_.Claim(active_->ID());
    ASSERT_FALSE(claim);
}

TEST_F(Test_ContactGroup, end)
{
    ot::ContactGroup::ItemMap::const_iterator it = contactGroup_.end();
    ASSERT_EQ(contactGroup_.begin(), it);
    ASSERT_EQ(std::distance(contactGroup_.begin(), it), 0);

    const auto& group1 = contactGroup_.AddItem(active_);
    it = group1.end();
    ASSERT_NE(group1.begin(), it);
    ASSERT_EQ(1, std::distance(group1.begin(), it));

    std::advance(it, -1);
    ASSERT_EQ(group1.begin(), it);
    ASSERT_EQ(std::distance(group1.begin(), it), 0);
}

TEST_F(Test_ContactGroup, HaveClaim_true)
{
    const auto& group1 = contactGroup_.AddItem(active_);

    ASSERT_TRUE(group1.HaveClaim(active_->ID()));
}

TEST_F(Test_ContactGroup, HaveClaim_false)
{
    ASSERT_FALSE(contactGroup_.HaveClaim(active_->ID()));
}

TEST_F(Test_ContactGroup, Delete)
{
    const auto& group1 = contactGroup_.AddItem(active_);
    ASSERT_TRUE(group1.HaveClaim(active_->ID()));

    // Add a second item to help testing the size after trying to delete twice.
    const auto& group2 = group1.AddItem(primary_);
    ASSERT_EQ(2, group2.Size());

    const auto& group3 = group2.Delete(active_->ID());
    // Verify the item was deleted.
    ASSERT_FALSE(group3.HaveClaim(active_->ID()));
    ASSERT_EQ(1, group3.Size());

    const auto& group4 = group3.Delete(active_->ID());
    // Verify trying to delete the item again didn't change anything.
    ASSERT_EQ(1, group4.Size());
}

TEST_F(Test_ContactGroup, Primary_group_has_primary)
{
    const auto& group1 = contactGroup_.AddItem(primary_);
    ASSERT_FALSE(group1.Primary().empty());
    ASSERT_EQ(primary_->ID(), group1.Primary());
}

TEST_F(Test_ContactGroup, Primary_no_primary)
{
    ASSERT_TRUE(contactGroup_.Primary().empty());
}

TEST_F(Test_ContactGroup, PrimaryClaim_found)
{
    const auto& group1 = contactGroup_.AddItem(primary_);

    const std::shared_ptr<ot::ContactItem>& primaryClaim =
        group1.PrimaryClaim();
    ASSERT_NE(nullptr, primaryClaim);
    ASSERT_EQ(primary_->ID(), primaryClaim->ID());
}

TEST_F(Test_ContactGroup, PrimaryClaim_notfound)
{
    const std::shared_ptr<ot::ContactItem>& primaryClaim =
        contactGroup_.PrimaryClaim();
    ASSERT_FALSE(primaryClaim);
}

TEST_F(Test_ContactGroup, SerializeTo)
{
    ot::proto::ContactSection contactSection1;
    contactSection1.set_name(ot::contact::internal::translate(
        ot::contact::ContactSectionName::Identifier));

    // Serialize without ids.
    const auto& group1 = contactGroup_.AddItem(active_);
    ASSERT_TRUE(group1.SerializeTo(contactSection1, false));
    ASSERT_EQ(group1.Size(), contactSection1.item_size());
    ASSERT_EQ(
        ot::contact::ContactSectionName::Identifier,
        ot::contact::internal::translate(contactSection1.name()));
    ot::proto::ContactItem item = contactSection1.item(0);
    ASSERT_EQ(active_->Version(), item.version());
    ASSERT_EQ(active_->Type(), ot::contact::internal::translate(item.type()));
    ASSERT_EQ(active_->Value(), item.value());
    ASSERT_EQ(active_->Start(), item.start());
    ASSERT_EQ(active_->End(), item.end());
    ASSERT_EQ(
        ot::contact::ContactItemAttribute::Active,
        ot::contact::internal::translate(item.attribute(0)));

    ot::proto::ContactSection contactSection2;
    contactSection2.set_name(ot::contact::internal::translate(
        ot::contact::ContactSectionName::Identifier));

    // Serialize with ids.
    ASSERT_TRUE(group1.SerializeTo(contactSection2, true));
    ASSERT_EQ(group1.Size(), contactSection2.item_size());
    ASSERT_EQ(
        ot::contact::ContactSectionName::Identifier,
        ot::contact::internal::translate(contactSection2.name()));
    item = contactSection2.item(0);
    ASSERT_EQ(active_->ID().str(), item.id());
    ASSERT_EQ(active_->Version(), item.version());
    ASSERT_EQ(active_->Type(), ot::contact::internal::translate(item.type()));
    ASSERT_EQ(active_->Value(), item.value());
    ASSERT_EQ(active_->Start(), item.start());
    ASSERT_EQ(active_->End(), item.end());
    ASSERT_EQ(
        ot::contact::ContactItemAttribute::Active,
        ot::contact::internal::translate(item.attribute(0)));

    // Serialize to the wrong section.
    ot::proto::ContactSection contactSection3;
    contactSection3.set_name(
        ot::proto::ContactSectionName::CONTACTSECTION_ADDRESS);
    ASSERT_FALSE(group1.SerializeTo(contactSection3, false));
}

TEST_F(Test_ContactGroup, Size)
{
    ASSERT_EQ(contactGroup_.Size(), 0);
    const auto& group1 = contactGroup_.AddItem(primary_);
    ASSERT_EQ(1, group1.Size());
    const auto& group2 = group1.AddItem(active_);
    ASSERT_EQ(2, group2.Size());
    const auto& group3 = group2.Delete(active_->ID());
    ASSERT_EQ(1, group3.Size());
}

TEST_F(Test_ContactGroup, Type)
{
    ASSERT_EQ(ot::contact::ContactItemType::Employee, contactGroup_.Type());
}
