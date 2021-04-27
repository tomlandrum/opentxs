// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <string>

#include "Helpers.hpp"
#include "opentxs/api/client/Activity.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/client/blockchain/HD.hpp"
#include "opentxs/api/client/blockchain/Subchain.hpp"
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/blockchain/block/bitcoin/Transaction.hpp"
#include "opentxs/contact/Contact.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/Types.hpp"

using Subchain = ot::api::client::blockchain::Subchain;

std::string txid_1_{};
std::string txid_2_{};
ot::Bip32Index first_index_{};
ot::Bip32Index second_index_{};
ot::Bip32Index third_index_{};
ot::Bip32Index fourth_index_{};

TEST_F(Test_BlockchainActivity, init)
{
    EXPECT_FALSE(nym_1_id().empty());
    EXPECT_FALSE(nym_2_id().empty());
    EXPECT_FALSE(account_1_id().empty());
    EXPECT_FALSE(account_2_id().empty());
    EXPECT_FALSE(contact_1_id().empty());
    EXPECT_FALSE(contact_2_id().empty());
    EXPECT_FALSE(contact_5_id().empty());
    EXPECT_FALSE(contact_6_id().empty());

    {
        const auto contact = api_.Contacts().Contact(contact_1_id());

        ASSERT_TRUE(contact);
        EXPECT_EQ(contact->Label(), nym_1_name_);
        EXPECT_EQ(api_.Contacts().ContactName(contact_1_id()), nym_1_name_);
    }

    {
        const auto contact = api_.Contacts().Contact(contact_2_id());

        ASSERT_TRUE(contact);
        EXPECT_EQ(contact->Label(), nym_2_name_);
        EXPECT_EQ(api_.Contacts().ContactName(contact_2_id()), nym_2_name_);
    }

    {
        const auto contact = api_.Contacts().Contact(contact_5_id());

        ASSERT_TRUE(contact);
        EXPECT_EQ(contact->Label(), contact_5_name_);
        EXPECT_EQ(api_.Contacts().ContactName(contact_5_id()), contact_5_name_);
    }

    {
        const auto contact = api_.Contacts().Contact(contact_6_id());

        ASSERT_TRUE(contact);
        EXPECT_EQ(contact->Label(), contact_6_name_);
        EXPECT_EQ(api_.Contacts().ContactName(contact_6_id()), contact_6_name_);
    }

    auto thread1 = api_.Activity().Thread(nym_1_id(), contact_3_id());
    auto thread2 = api_.Activity().Thread(nym_1_id(), contact_4_id());
    auto thread3 = api_.Activity().Thread(nym_1_id(), contact_5_id());
    auto thread4 = api_.Activity().Thread(nym_1_id(), contact_6_id());

    EXPECT_FALSE(thread1);
    EXPECT_FALSE(thread2);
    EXPECT_FALSE(thread3);
    EXPECT_FALSE(thread4);
}

TEST_F(Test_BlockchainActivity, setup)
{
    const auto& account =
        api_.Blockchain().HDSubaccount(nym_1_id(), account_1_id());
    const auto indexOne = account.Reserve(Subchain::External, reason_);
    const auto indexTwo = account.Reserve(Subchain::External, reason_);
    const auto indexThree = account.Reserve(Subchain::External, reason_);
    const auto indexFour = account.Reserve(Subchain::External, reason_);

    ASSERT_TRUE(indexOne.has_value());
    ASSERT_TRUE(indexTwo.has_value());
    ASSERT_TRUE(indexThree.has_value());
    ASSERT_TRUE(indexFour.has_value());

    first_index_ = indexOne.value();
    second_index_ = indexTwo.value();
    third_index_ = indexThree.value();
    fourth_index_ = indexFour.value();
    const auto& keyOne =
        account.BalanceElement(Subchain::External, first_index_);
    const auto& keyTwo =
        account.BalanceElement(Subchain::External, second_index_);
    const auto& keyThree =
        account.BalanceElement(Subchain::External, third_index_);
    const auto& keyFour =
        account.BalanceElement(Subchain::External, fourth_index_);

    EXPECT_TRUE(api_.Blockchain().AssignContact(
        nym_1_id(),
        account_1_id(),
        Subchain::External,
        first_index_,
        contact_5_id()));
    EXPECT_TRUE(api_.Blockchain().AssignContact(
        nym_1_id(),
        account_1_id(),
        Subchain::External,
        third_index_,
        contact_6_id()));

    const auto tx1 = get_test_transaction(keyOne, keyTwo);
    const auto tx2 = get_test_transaction(keyThree, keyFour);

    ASSERT_TRUE(tx1);
    ASSERT_TRUE(tx2);

    txid_1_ = tx1->ID().asHex();
    txid_2_ = tx2->ID().asHex();

    ASSERT_TRUE(api_.Blockchain().ProcessTransaction(
        ot::blockchain::Type::Bitcoin, *tx1, reason_));
    ASSERT_TRUE(api_.Blockchain().ProcessTransaction(
        ot::blockchain::Type::Bitcoin, *tx2, reason_));

    auto thread1 = api_.Activity().Thread(nym_1_id(), contact_5_id());
    auto thread2 = api_.Activity().Thread(nym_1_id(), contact_6_id());

    ASSERT_TRUE(thread1);
    ASSERT_TRUE(thread2);

    //    EXPECT_TRUE(check_thread(*thread1, txid_1_));
    //    EXPECT_TRUE(check_thread(*thread2, txid_2_));
}

TEST_F(Test_BlockchainActivity, merge)
{
    // TODO ASSERT_TRUE(api_.Contacts().Merge(contact_5_id(), contact_6_id()));
}
