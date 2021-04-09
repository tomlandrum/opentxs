// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <atomic>
#include <future>
#include <string>
#include <utility>

#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "UIHelpers.hpp"
#include "integration/Helpers.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/client/OTX.hpp"
#include "opentxs/api/client/UI.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/otx/LastReplyStatus.hpp"
#include "opentxs/ui/ContactList.hpp"
#include "opentxs/ui/ContactListItem.hpp"
#include "opentxs/ui/MessagableList.hpp"

namespace opentxs
{
namespace api
{
namespace server
{
class Manager;
}  // namespace server
}  // namespace api
}  // namespace opentxs

namespace
{
Counter contact_list_alex_{};
Counter messagable_list_alex_{};
Counter contact_list_bob_{};
Counter messagable_list_bob_{};
Counter contact_list_chris_{};
Counter messagable_list_chris_{};

struct Test_AddContact : public ::testing::Test {
    const ot::api::client::Manager& api_alex_;
    const ot::api::client::Manager& api_bob_;
    const ot::api::client::Manager& api_chris_;
    const ot::api::server::Manager& api_server_1_;

    Test_AddContact()
        : api_alex_(ot::Context().StartClient(OTTestEnvironment::test_args_, 0))
        , api_bob_(ot::Context().StartClient(OTTestEnvironment::test_args_, 1))
        , api_chris_(
              ot::Context().StartClient(OTTestEnvironment::test_args_, 2))
        , api_server_1_(
              ot::Context().StartServer(OTTestEnvironment::test_args_, 0, true))
    {
        const_cast<Server&>(server_1_).init(api_server_1_);
        const_cast<User&>(alex_).init(api_alex_, server_1_);
        const_cast<User&>(bob_).init(api_bob_, server_1_);
        const_cast<User&>(chris_).init(api_chris_, server_1_);
    }
};

TEST_F(Test_AddContact, init_ot) {}

TEST_F(Test_AddContact, init_ui)
{
    contact_list_alex_.expected_ = 1;
    messagable_list_alex_.expected_ = 0;
    contact_list_bob_.expected_ = 1;
    messagable_list_bob_.expected_ = 0;
    contact_list_chris_.expected_ = 1;
    messagable_list_chris_.expected_ = 0;
    api_alex_.UI().ContactList(
        alex_.nym_id_, make_cb(contact_list_alex_, "alex contact list"));
    api_alex_.UI().MessagableList(
        alex_.nym_id_, make_cb(messagable_list_alex_, "alex messagable list"));
    api_bob_.UI().ContactList(
        bob_.nym_id_, make_cb(contact_list_bob_, "bob contact list"));
    api_bob_.UI().MessagableList(
        bob_.nym_id_, make_cb(messagable_list_bob_, "bob messagable list"));
    api_chris_.UI().ContactList(
        chris_.nym_id_, make_cb(contact_list_chris_, "chris contact list"));
    api_chris_.UI().MessagableList(
        chris_.nym_id_,
        make_cb(messagable_list_chris_, "chris messagable list"));
}

TEST_F(Test_AddContact, initial_state_contact_list_alex)
{
    ASSERT_TRUE(wait_for_counter(contact_list_alex_));

    const auto& widget = alex_.api_->UI().ContactList(alex_.nym_id_);
    const auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(
        row->DisplayName() == alex_.name_ || row->DisplayName() == "Owner");
    ASSERT_TRUE(row->Valid());
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("ME", row->Section().c_str());
    EXPECT_TRUE(row->Last());

    EXPECT_TRUE(alex_.SetContact(alex_.name_, row->ContactID()));
    EXPECT_FALSE(alex_.Contact(alex_.name_).empty());
}

TEST_F(Test_AddContact, initial_state_messagable_list_alex)
{
    ASSERT_TRUE(wait_for_counter(messagable_list_alex_));

    const auto& widget = alex_.api_->UI().MessagableList(alex_.nym_id_);
    auto row = widget.First();

    EXPECT_FALSE(row->Valid());
}

TEST_F(Test_AddContact, initial_state_contact_list_bob)
{
    ASSERT_TRUE(wait_for_counter(contact_list_bob_));

    const auto& widget = bob_.api_->UI().ContactList(bob_.nym_id_);
    const auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_TRUE(
        row->DisplayName() == bob_.name_ || row->DisplayName() == "Owner");
    ASSERT_TRUE(row->Valid());
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("ME", row->Section().c_str());
    EXPECT_TRUE(row->Last());

    EXPECT_TRUE(bob_.SetContact(bob_.name_, row->ContactID()));
    EXPECT_FALSE(bob_.Contact(bob_.name_).empty());
}

TEST_F(Test_AddContact, initial_state_messagable_list_bob)
{
    ASSERT_TRUE(wait_for_counter(messagable_list_bob_));

    const auto& widget = bob_.api_->UI().MessagableList(bob_.nym_id_);
    auto row = widget.First();

    EXPECT_FALSE(row->Valid());
}

TEST_F(Test_AddContact, initial_state_contact_list_chris)
{
    ASSERT_TRUE(wait_for_counter(contact_list_chris_));

    const auto& widget = chris_.api_->UI().ContactList(chris_.nym_id_);
    const auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_TRUE(
        row->DisplayName() == chris_.name_ || row->DisplayName() == "Owner");
    ASSERT_TRUE(row->Valid());
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("ME", row->Section().c_str());
    EXPECT_TRUE(row->Last());

    EXPECT_TRUE(chris_.SetContact(chris_.name_, row->ContactID()));
    EXPECT_FALSE(chris_.Contact(chris_.name_).empty());
}

TEST_F(Test_AddContact, initial_state_messagable_list_chris)
{
    ASSERT_TRUE(wait_for_counter(messagable_list_chris_));

    const auto& widget = chris_.api_->UI().MessagableList(chris_.nym_id_);
    auto row = widget.First();

    EXPECT_FALSE(row->Valid());
}

TEST_F(Test_AddContact, introduction_server)
{
    api_alex_.OTX().StartIntroductionServer(alex_.nym_id_);
    api_bob_.OTX().StartIntroductionServer(bob_.nym_id_);
    api_chris_.OTX().StartIntroductionServer(chris_.nym_id_);
    auto alexTask =
        api_alex_.OTX().RegisterNymPublic(alex_.nym_id_, server_1_.id_, true);
    auto bobTask =
        api_bob_.OTX().RegisterNymPublic(bob_.nym_id_, server_1_.id_, true);
    auto chrisTask =
        api_chris_.OTX().RegisterNymPublic(chris_.nym_id_, server_1_.id_, true);

    ASSERT_NE(0, alexTask.first);
    ASSERT_NE(0, bobTask.first);
    ASSERT_NE(0, chrisTask.first);
    EXPECT_EQ(
        ot::otx::LastReplyStatus::MessageSuccess, alexTask.second.get().first);
    EXPECT_EQ(
        ot::otx::LastReplyStatus::MessageSuccess, bobTask.second.get().first);
    EXPECT_EQ(
        ot::otx::LastReplyStatus::MessageSuccess, chrisTask.second.get().first);

    api_alex_.OTX().ContextIdle(alex_.nym_id_, server_1_.id_).get();
    api_bob_.OTX().ContextIdle(bob_.nym_id_, server_1_.id_).get();
    api_chris_.OTX().ContextIdle(chris_.nym_id_, server_1_.id_).get();
}

TEST_F(Test_AddContact, nymid)
{
    ASSERT_FALSE(bob_.nym_id_->empty());

    contact_list_alex_.expected_ += 1;
    messagable_list_alex_.expected_ += 1;
    const auto& widget = alex_.api_->UI().ContactList(alex_.nym_id_);
    const auto id = widget.AddContact(bob_.name_, bob_.nym_id_->str(), "");

    EXPECT_FALSE(id.empty());
}

TEST_F(Test_AddContact, add_nymid_contact_list_alex)
{
    ASSERT_TRUE(wait_for_counter(contact_list_alex_));

    const auto& widget = alex_.api_->UI().ContactList(alex_.nym_id_);
    auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == alex_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("ME", row->Section().c_str());
    ASSERT_FALSE(row->Last());

    row = widget.Next();

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == bob_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("B", row->Section().c_str());
    EXPECT_TRUE(row->Last());

    EXPECT_TRUE(alex_.SetContact(bob_.name_, row->ContactID()));
    EXPECT_FALSE(alex_.Contact(bob_.name_).empty());
}

TEST_F(Test_AddContact, add_nymid_messagable_list_alex)
{
    ASSERT_TRUE(wait_for_counter(messagable_list_alex_));

    const auto& widget = alex_.api_->UI().MessagableList(alex_.nym_id_);
    auto row = widget.First();

    ASSERT_TRUE(row->Valid());

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_EQ(row->DisplayName(), bob_.name_);
    ASSERT_TRUE(row->Valid());
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("B", row->Section().c_str());
    EXPECT_TRUE(row->Last());
}

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
TEST_F(Test_AddContact, paymentcode)
{
    ASSERT_FALSE(chris_.payment_code_.empty());

    contact_list_alex_.expected_ += 1;
    messagable_list_alex_.expected_ += 1;
    const auto& widget = alex_.api_->UI().ContactList(alex_.nym_id_);
    const auto id = widget.AddContact(chris_.name_, "", chris_.payment_code_);

    EXPECT_FALSE(id.empty());
}

TEST_F(Test_AddContact, add_paymentcode_contact_list_alex)
{
    ASSERT_TRUE(wait_for_counter(contact_list_alex_));

    const auto& widget = alex_.api_->UI().ContactList(alex_.nym_id_);
    auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == alex_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("ME", row->Section().c_str());
    ASSERT_FALSE(row->Last());

    row = widget.Next();

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == bob_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("B", row->Section().c_str());
    ASSERT_FALSE(row->Last());

    row = widget.Next();

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == chris_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("C", row->Section().c_str());
    EXPECT_TRUE(row->Last());

    EXPECT_TRUE(alex_.SetContact(chris_.name_, row->ContactID()));
    EXPECT_FALSE(alex_.Contact(chris_.name_).empty());
}

TEST_F(Test_AddContact, add_paymentcode_messagable_list_alex)
{
    ASSERT_TRUE(wait_for_counter(messagable_list_alex_));

    const auto& widget = alex_.api_->UI().MessagableList(alex_.nym_id_);
    auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == bob_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("B", row->Section().c_str());
    ASSERT_FALSE(row->Last());

    row = widget.Next();

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == chris_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("C", row->Section().c_str());
    EXPECT_TRUE(row->Last());
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32

TEST_F(Test_AddContact, both)
{
    ASSERT_FALSE(alex_.nym_id_->empty());
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
    ASSERT_FALSE(alex_.payment_code_.empty());
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32

    contact_list_bob_.expected_ += 1;
    messagable_list_bob_.expected_ += 1;
    const auto& widget = bob_.api_->UI().ContactList(bob_.nym_id_);
    const auto id = widget.AddContact(
        alex_.name_, alex_.nym_id_->str(), alex_.payment_code_);

    EXPECT_FALSE(id.empty());
}

TEST_F(Test_AddContact, add_both_contact_list_bob)
{
    ASSERT_TRUE(wait_for_counter(contact_list_bob_));

    const auto& widget = bob_.api_->UI().ContactList(bob_.nym_id_);
    auto row = widget.First();

    ASSERT_TRUE(row->Valid());

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == bob_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("ME", row->Section().c_str());
    ASSERT_FALSE(row->Last());

    row = widget.Next();

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == alex_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("A", row->Section().c_str());
    EXPECT_TRUE(row->Last());

    EXPECT_TRUE(bob_.SetContact(alex_.name_, row->ContactID()));
    EXPECT_FALSE(bob_.Contact(alex_.name_).empty());
}

TEST_F(Test_AddContact, add_both_messagable_list_bob)
{
    ASSERT_TRUE(wait_for_counter(messagable_list_bob_));

    const auto& widget = bob_.api_->UI().MessagableList(bob_.nym_id_);
    auto row = widget.First();

    ASSERT_TRUE(row->Valid());

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_EQ(row->DisplayName(), alex_.name_);
    ASSERT_TRUE(row->Valid());
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("A", row->Section().c_str());
    EXPECT_TRUE(row->Last());
}

TEST_F(Test_AddContact, backwards)
{
    ASSERT_FALSE(chris_.nym_id_->empty());
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
    ASSERT_FALSE(chris_.payment_code_.empty());
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32

    contact_list_bob_.expected_ += 1;
    messagable_list_bob_.expected_ += 1;
    const auto& widget = bob_.api_->UI().ContactList(bob_.nym_id_);
    const auto id = widget.AddContact(
        chris_.name_, chris_.payment_code_, chris_.nym_id_->str());

    EXPECT_FALSE(id.empty());
}

TEST_F(Test_AddContact, add_backwards_contact_list_bob)
{
    ASSERT_TRUE(wait_for_counter(contact_list_bob_));

    const auto& widget = bob_.api_->UI().ContactList(bob_.nym_id_);
    auto row = widget.First();

    ASSERT_TRUE(row->Valid());

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == bob_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("ME", row->Section().c_str());
    ASSERT_FALSE(row->Last());

    row = widget.Next();

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == alex_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("A", row->Section().c_str());
    ASSERT_FALSE(row->Last());

    row = widget.Next();

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == chris_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("C", row->Section().c_str());
    EXPECT_TRUE(row->Last());

    EXPECT_TRUE(bob_.SetContact(chris_.name_, row->ContactID()));
    EXPECT_FALSE(bob_.Contact(chris_.name_).empty());
}

TEST_F(Test_AddContact, add_backwards_messagable_list_bob)
{
    ASSERT_TRUE(wait_for_counter(messagable_list_bob_));

    const auto& widget = bob_.api_->UI().MessagableList(bob_.nym_id_);
    auto row = widget.First();

    ASSERT_TRUE(row->Valid());

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == alex_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("A", row->Section().c_str());
    ASSERT_FALSE(row->Last());

    row = widget.Next();

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == chris_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("C", row->Section().c_str());
    EXPECT_TRUE(row->Last());
}

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
TEST_F(Test_AddContact, paymentcode_as_nymid)
{
    ASSERT_FALSE(alex_.payment_code_.empty());

    contact_list_chris_.expected_ += 1;
    messagable_list_chris_.expected_ += 1;
    const auto& widget = chris_.api_->UI().ContactList(chris_.nym_id_);
    const auto id = widget.AddContact(alex_.name_, alex_.payment_code_, "");

    EXPECT_FALSE(id.empty());
}

TEST_F(Test_AddContact, add_payment_code_as_nymid_contact_list_chris)
{
    ASSERT_TRUE(wait_for_counter(contact_list_chris_));

    const auto& widget = chris_.api_->UI().ContactList(chris_.nym_id_);
    auto row = widget.First();

    ASSERT_TRUE(row->Valid());

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == chris_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("ME", row->Section().c_str());
    ASSERT_FALSE(row->Last());

    row = widget.Next();

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == alex_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("A", row->Section().c_str());
    EXPECT_TRUE(row->Last());

    EXPECT_TRUE(chris_.SetContact(alex_.name_, row->ContactID()));
    EXPECT_FALSE(chris_.Contact(alex_.name_).empty());
}

TEST_F(Test_AddContact, add_payment_code_as_nymid_messagable_list_chris)
{
    ASSERT_TRUE(wait_for_counter(messagable_list_chris_));

    const auto& widget = chris_.api_->UI().MessagableList(chris_.nym_id_);
    auto row = widget.First();

    ASSERT_TRUE(row->Valid());

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_EQ(row->DisplayName(), alex_.name_);
    ASSERT_TRUE(row->Valid());
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("A", row->Section().c_str());
    EXPECT_TRUE(row->Last());
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32

TEST_F(Test_AddContact, nymid_as_paymentcode)
{
    ASSERT_FALSE(bob_.nym_id_->empty());

    contact_list_chris_.expected_ += 1;
    messagable_list_chris_.expected_ += 1;
    const auto& widget = chris_.api_->UI().ContactList(chris_.nym_id_);
    const auto id = widget.AddContact(bob_.name_, "", bob_.nym_id_->str());

    EXPECT_FALSE(id.empty());
}

TEST_F(Test_AddContact, add_nymid_as_paymentcode_contact_list_chris)
{
    ASSERT_TRUE(wait_for_counter(contact_list_chris_));

    const auto& widget = chris_.api_->UI().ContactList(chris_.nym_id_);
    auto row = widget.First();

    ASSERT_TRUE(row->Valid());

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == chris_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("ME", row->Section().c_str());
    ASSERT_FALSE(row->Last());

    row = widget.Next();

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == alex_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("A", row->Section().c_str());
    ASSERT_FALSE(row->Last());

    row = widget.Next();
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == bob_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("B", row->Section().c_str());
    EXPECT_TRUE(row->Last());

    EXPECT_TRUE(chris_.SetContact(bob_.name_, row->ContactID()));
    EXPECT_FALSE(chris_.Contact(bob_.name_).empty());
}

TEST_F(Test_AddContact, add_nymid_as_paymentcode_messagable_list_chris)
{
    ASSERT_TRUE(wait_for_counter(messagable_list_chris_));

    const auto& widget = chris_.api_->UI().MessagableList(chris_.nym_id_);
    auto row = widget.First();

    ASSERT_TRUE(row->Valid());

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == alex_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("A", row->Section().c_str());
    ASSERT_FALSE(row->Last());

    row = widget.Next();
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32

    EXPECT_FALSE(row->ContactID().empty());
    EXPECT_TRUE(row->DisplayName() == bob_.name_);
    EXPECT_STREQ("", row->ImageURI().c_str());
    EXPECT_STREQ("B", row->Section().c_str());
    EXPECT_TRUE(row->Last());
}

TEST_F(Test_AddContact, shutdown)
{
    api_alex_.OTX().ContextIdle(alex_.nym_id_, server_1_.id_).get();
    api_bob_.OTX().ContextIdle(bob_.nym_id_, server_1_.id_).get();
    api_chris_.OTX().ContextIdle(chris_.nym_id_, server_1_.id_).get();

    EXPECT_EQ(contact_list_alex_.expected_, contact_list_alex_.updated_);
    EXPECT_EQ(messagable_list_alex_.expected_, messagable_list_alex_.updated_);
    EXPECT_EQ(contact_list_bob_.expected_, contact_list_bob_.updated_);
    EXPECT_EQ(messagable_list_bob_.expected_, messagable_list_bob_.updated_);
    EXPECT_EQ(contact_list_chris_.expected_, contact_list_chris_.updated_);
    EXPECT_EQ(
        messagable_list_chris_.expected_, messagable_list_chris_.updated_);
}
}  // namespace
