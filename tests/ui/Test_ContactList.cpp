// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <atomic>
#include <memory>
#include <string>

#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "UIHelpers.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/client/UI.hpp"
#include "opentxs/client/OTAPI_Exec.hpp"
#include "opentxs/contact/Contact.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/ui/ContactList.hpp"
#include "opentxs/ui/ContactListItem.hpp"

#define ALICE_NYM_ID "otysEksLyHmDZyPDTM4TYvcXEcu18bnZw5V"
#define ALICE_NYM_NAME "Alice"
#define BOB_PAYMENT_CODE                                                       \
    "PM8TJS2JxQ5ztXUpBBRnpTbcUXbUHy2T1abfrb3KkAAtMEGNbey4oumH7Hc578WgQJhPjBxt" \
    "eQ5GHHToTYHE3A1w6p7tU6KSoFmWBVbFGjKPisZDbP97"
#define BOB_NYM_NAME "Bob"
#define CHRIS_PAYMENT_CODE                                                     \
    "PM8TJfV1DQD6VScd5AWsSax8RgK9cUREe939M1d85MwGCKJukyghX6B5E7kqcCyEYu6Tu1Zv" \
    "dG8aWh6w8KGhSfjgL8fBKuZS6aUjhV9xLV1R16CcgWhw"
#define CHRIS_NYM_NAME "Chris"

namespace
{
Counter contact_list_widget_{1, 0};

class Test_ContactList : public ::testing::Test
{
public:
    const ot::api::client::Manager& client_;
    ot::OTPasswordPrompt reason_;
    const std::string fingerprint_;
    const ot::OTNymID nym_id_;
    const ot::ui::ContactList& contact_list_;
    std::atomic<bool> shutdown_;
    const ot::OTPaymentCode bob_payment_code_;
    ot::OTIdentifier bob_contact_id_;
    const ot::OTPaymentCode chris_payment_code_;
    ot::OTIdentifier chris_contact_id_;

    Test_ContactList()
        : client_(ot::Context().StartClient({}, 0))
        , reason_(client_.Factory().PasswordPrompt(__FUNCTION__))
        , fingerprint_(client_.Exec().Wallet_ImportSeed(
              "response seminar brave tip suit recall often sound stick owner "
              "lottery motion",
              ""))
        , nym_id_(client_.Wallet()
                      .Nym(reason_, ALICE_NYM_NAME, {fingerprint_, 0})
                      ->ID())
        , contact_list_(client_.UI().ContactList(
              nym_id_,
              make_cb(contact_list_widget_, "contact list")))
        , shutdown_(false)
        , bob_payment_code_(
              client_.Factory().PaymentCode(std::string{BOB_PAYMENT_CODE}))
        , bob_contact_id_(ot::Identifier::Factory())
        , chris_payment_code_(
              client_.Factory().PaymentCode(std::string{CHRIS_PAYMENT_CODE}))
        , chris_contact_id_(ot::Identifier::Factory())
    {
    }
};

TEST_F(Test_ContactList, initialize_opentxs)
{
    ASSERT_FALSE(nym_id_->empty());
    ASSERT_EQ(nym_id_->str(), ALICE_NYM_ID);
    ASSERT_TRUE(bob_payment_code_->Valid());
}

TEST_F(Test_ContactList, initial_state)
{
    ASSERT_TRUE(wait_for_counter(contact_list_widget_));

    const auto me = contact_list_.First();

    ASSERT_TRUE(me.get().Valid());
    EXPECT_EQ(me.get().DisplayName(), ALICE_NYM_NAME);
    EXPECT_TRUE(me.get().Last());
}

TEST_F(Test_ContactList, add_bob)
{
    contact_list_widget_.expected_ += 1;
    const auto bob = client_.Contacts().NewContact(
        BOB_NYM_NAME, bob_payment_code_->ID(), bob_payment_code_);

    ASSERT_TRUE(bob);

    bob_contact_id_ = bob->ID();

    ASSERT_FALSE(bob_contact_id_->empty());
}

TEST_F(Test_ContactList, add_bob_state)
{
    ASSERT_TRUE(wait_for_counter(contact_list_widget_));

    const auto me = contact_list_.First();

    ASSERT_TRUE(me.get().Valid());
    EXPECT_EQ(me.get().DisplayName(), ALICE_NYM_NAME);
    ASSERT_FALSE(me.get().Last());

    const auto bob = contact_list_.Next();

    EXPECT_EQ(bob.get().DisplayName(), BOB_NYM_NAME);
    EXPECT_TRUE(bob.get().Last());
}

TEST_F(Test_ContactList, add_chris)
{
    contact_list_widget_.expected_ += 1;
    const auto chris = client_.Contacts().NewContact(
        CHRIS_NYM_NAME, chris_payment_code_->ID(), chris_payment_code_);

    ASSERT_TRUE(chris);

    chris_contact_id_ = chris->ID();
}

TEST_F(Test_ContactList, add_chris_state)
{
    ASSERT_TRUE(wait_for_counter(contact_list_widget_));

    const auto me = contact_list_.First();

    ASSERT_TRUE(me.get().Valid());
    EXPECT_EQ(me.get().DisplayName(), ALICE_NYM_NAME);
    ASSERT_FALSE(me.get().Last());

    const auto bob = contact_list_.Next();

    EXPECT_EQ(bob.get().DisplayName(), BOB_NYM_NAME);
    ASSERT_FALSE(bob.get().Last());

    const auto chris = contact_list_.Next();

    EXPECT_EQ(chris.get().DisplayName(), CHRIS_NYM_NAME);
    EXPECT_TRUE(chris.get().Last());
}

TEST_F(Test_ContactList, shutdown)
{
    EXPECT_EQ(contact_list_widget_.expected_, contact_list_widget_.updated_);
}
}  // namespace
