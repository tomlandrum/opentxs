// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "VectorsV3.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/HDSeed.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/client/blockchain/BalanceNode.hpp"
#include "opentxs/api/client/blockchain/PaymentCode.hpp"
#include "opentxs/api/client/blockchain/Subchain.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/crypto/Language.hpp"
#include "opentxs/crypto/SeedStyle.hpp"
#include "opentxs/crypto/key/EllipticCurve.hpp"
#include "opentxs/identity/Nym.hpp"

namespace ot = opentxs;

namespace
{
class Test_PaymentCodeAPI : public ::testing::Test
{
public:
    const ot::api::client::Manager& alice_;
    const ot::api::client::Manager& bob_;

    Test_PaymentCodeAPI()
        : alice_(ot::Context().StartClient(OTTestEnvironment::test_args_, 0))
        , bob_(ot::Context().StartClient(OTTestEnvironment::test_args_, 1))
    {
    }
};

TEST_F(Test_PaymentCodeAPI, init) {}

TEST_F(Test_PaymentCodeAPI, alice)
{
    const auto& vector = vectors_3_.alice_;
    const auto& remote = vectors_3_.bob_;
    constexpr auto receiveChain{ot::blockchain::Type::Bitcoin};
    constexpr auto sendChain{ot::blockchain::Type::Bitcoin_testnet3};
    const auto reason = alice_.Factory().PasswordPrompt(__FUNCTION__);
    const auto seedID = [&] {
        const auto words = alice_.Factory().SecretFromText(vector.words_);
        const auto phrase = alice_.Factory().Secret(0);

        return alice_.Seeds().ImportSeed(
            words,
            phrase,
            ot::crypto::SeedStyle::BIP39,
            ot::crypto::Language::en,
            reason);
    }();

    EXPECT_FALSE(seedID.empty());

    const auto pNym = alice_.Wallet().Nym(reason, "Alice", {seedID, 0});

    ASSERT_TRUE(pNym);

    const auto& nym = *pNym;
    const auto localPC = alice_.Factory().PaymentCode(nym.PaymentCode());
    const auto remotePC = alice_.Factory().PaymentCode(remote.payment_code_);

    EXPECT_EQ(localPC->Version(), 3);
    EXPECT_EQ(remotePC->Version(), 3);
    EXPECT_EQ(localPC->asBase58(), vector.payment_code_);

    const auto path = [&] {
        auto out = ot::Space{};
        nym.PaymentCodePath(ot::writer(out));

        return out;
    }();
    const auto id1 = alice_.Blockchain().NewPaymentCodeSubaccount(
        nym.ID(), localPC, remotePC, ot::reader(path), receiveChain, reason);
    const auto id2 = alice_.Blockchain().NewPaymentCodeSubaccount(
        nym.ID(), localPC, remotePC, ot::reader(path), sendChain, reason);

    ASSERT_FALSE(id1->empty());
    ASSERT_FALSE(id2->empty());

    const auto& account1 =
        alice_.Blockchain().PaymentCodeSubaccount(nym.ID(), id1);
    const auto& account2 =
        alice_.Blockchain().PaymentCodeSubaccount(nym.ID(), id2);
    using Subchain = ot::api::client::blockchain::Subchain;
    const auto populate = [&](const auto& account, const auto& target) {
        const auto generate = [&](const auto subchain) {
            auto index = account.LastGenerated(subchain);

            while (!index.has_value() || index.value() < target.size()) {
                index = account.GenerateNext(subchain, reason);
            }
        };

        generate(Subchain::Incoming);
        generate(Subchain::Outgoing);
    };
    populate(account1, vector.receive_keys_);
    populate(account2, remote.receive_keys_);

    for (auto i{0u}; i < vector.receive_keys_.size(); ++i) {
        const auto expected = alice_.Factory().Data(
            vector.receive_keys_.at(i), ot::StringStyle::Hex);
        const auto& element = account1.BalanceElement(Subchain::Incoming, i);
        const auto pKey = element.Key();

        ASSERT_TRUE(pKey);

        const auto& key = *pKey;

        EXPECT_EQ(expected->Bytes(), key.PublicKey());

        const auto& element2 = alice_.Blockchain().GetKey(element.KeyID());

        EXPECT_EQ(element.KeyID(), element2.KeyID());
    }

    for (auto i{0u}; i < remote.receive_keys_.size(); ++i) {
        const auto expected = alice_.Factory().Data(
            remote.receive_keys_.at(i), ot::StringStyle::Hex);
        const auto& element = account2.BalanceElement(Subchain::Outgoing, i);
        const auto pKey = element.Key();

        ASSERT_TRUE(pKey);

        const auto& key = *pKey;

        EXPECT_EQ(expected->Bytes(), key.PublicKey());

        const auto& element2 = alice_.Blockchain().GetKey(element.KeyID());

        EXPECT_EQ(element.KeyID(), element2.KeyID());
    }
}

TEST_F(Test_PaymentCodeAPI, bob)
{
    const auto& vector = vectors_3_.bob_;
    const auto& remote = vectors_3_.alice_;
    constexpr auto receiveChain{ot::blockchain::Type::Bitcoin_testnet3};
    constexpr auto sendChain{ot::blockchain::Type::Bitcoin};
    const auto reason = bob_.Factory().PasswordPrompt(__FUNCTION__);
    const auto seedID = [&] {
        const auto words = bob_.Factory().SecretFromText(vector.words_);
        const auto phrase = bob_.Factory().Secret(0);

        return bob_.Seeds().ImportSeed(
            words,
            phrase,
            ot::crypto::SeedStyle::BIP39,
            ot::crypto::Language::en,
            reason);
    }();

    EXPECT_FALSE(seedID.empty());

    const auto pNym = bob_.Wallet().Nym(reason, "Bob", {seedID, 0});

    ASSERT_TRUE(pNym);

    const auto& nym = *pNym;
    const auto localPC = bob_.Factory().PaymentCode(nym.PaymentCode());
    const auto remotePC = bob_.Factory().PaymentCode(remote.payment_code_);

    EXPECT_EQ(localPC->Version(), 3);
    EXPECT_EQ(remotePC->Version(), 3);
    EXPECT_EQ(localPC->asBase58(), vector.payment_code_);

    const auto path = [&] {
        auto out = ot::Space{};
        nym.PaymentCodePath(ot::writer(out));

        return out;
    }();
    const auto id1 = bob_.Blockchain().NewPaymentCodeSubaccount(
        nym.ID(), localPC, remotePC, ot::reader(path), receiveChain, reason);
    const auto id2 = bob_.Blockchain().NewPaymentCodeSubaccount(
        nym.ID(), localPC, remotePC, ot::reader(path), sendChain, reason);

    ASSERT_FALSE(id1->empty());
    ASSERT_FALSE(id2->empty());

    const auto& account1 =
        bob_.Blockchain().PaymentCodeSubaccount(nym.ID(), id1);
    const auto& account2 =
        bob_.Blockchain().PaymentCodeSubaccount(nym.ID(), id2);
    using Subchain = ot::api::client::blockchain::Subchain;
    const auto populate = [&](const auto& account, const auto& target) {
        const auto generate = [&](const auto subchain) {
            auto index = account.LastGenerated(subchain);

            while (!index.has_value() || index.value() < target.size()) {
                index = account.GenerateNext(subchain, reason);
            }
        };

        generate(Subchain::Incoming);
        generate(Subchain::Outgoing);
    };
    populate(account1, vector.receive_keys_);
    populate(account2, remote.receive_keys_);

    for (auto i{0u}; i < vector.receive_keys_.size(); ++i) {
        const auto expected = bob_.Factory().Data(
            vector.receive_keys_.at(i), ot::StringStyle::Hex);
        const auto& element = account1.BalanceElement(Subchain::Incoming, i);
        const auto pKey = element.Key();

        ASSERT_TRUE(pKey);

        const auto& key = *pKey;

        EXPECT_EQ(expected->Bytes(), key.PublicKey());

        const auto& element2 = bob_.Blockchain().GetKey(element.KeyID());

        EXPECT_EQ(element.KeyID(), element2.KeyID());
    }

    for (auto i{0u}; i < remote.receive_keys_.size(); ++i) {
        const auto expected = bob_.Factory().Data(
            remote.receive_keys_.at(i), ot::StringStyle::Hex);
        const auto& element = account2.BalanceElement(Subchain::Outgoing, i);
        const auto pKey = element.Key();

        ASSERT_TRUE(pKey);

        const auto& key = *pKey;

        EXPECT_EQ(expected->Bytes(), key.PublicKey());

        const auto& element2 = bob_.Blockchain().GetKey(element.KeyID());

        EXPECT_EQ(element.KeyID(), element2.KeyID());
    }
}
}  // namespace
