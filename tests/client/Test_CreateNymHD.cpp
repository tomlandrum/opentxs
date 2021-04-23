// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "opentxs/OT.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/client/OTAPI_Exec.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/Bip32Child.hpp"
#include "opentxs/crypto/Bip43Purpose.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/identity/Nym.hpp"

using namespace opentxs;

namespace
{
#if OT_CRYPTO_WITH_BIP32
class Test_CreateNymHD : public ::testing::Test
{
public:
    const ot::api::client::Manager& client_;
    ot::OTPasswordPrompt reason_;
    std::string SeedA_;
    std::string SeedB_;
    std::string SeedC_;
    std::string SeedD_;
    std::string AliceID, BobID, EveID, FrankID;
    std::string Alice, Bob;

    Test_CreateNymHD()
        : client_(ot::Context().StartClient({}, 0))
        , reason_(client_.Factory().PasswordPrompt(__FUNCTION__))
        // these fingerprints are deterministic so we can share them among tests
        , SeedA_(client_.Exec().Wallet_ImportSeed(
              "spike nominee miss inquiry fee nothing belt list other daughter "
              "leave valley twelve gossip paper",
              ""))
        , SeedB_(client_.Exec().Wallet_ImportSeed(
              "glimpse destroy nation advice seven useless candy move number "
              "toast insane anxiety proof enjoy lumber",
              ""))
        , SeedC_(client_.Exec().Wallet_ImportSeed("park cabbage quit", ""))
        , SeedD_(client_.Exec().Wallet_ImportSeed("federal dilemma rare", ""))
        , AliceID("ot24XFA1wKynjaAB59dx7PwEzGg37U8Q2yXG")
        , BobID("ot274uRuN1VezD47R7SqAH27s2WKP1U5jKWk")
        , EveID("otwz4jCuiVg7UF2i1NgCSvTWeDS29EAHeL6")
        , FrankID("otogQecfWnoJn5Juy5z5Si3mS53rTb7LgGe")
        , Alice(client_.Wallet()
                    .Nym(reason_, "Alice", {SeedA_, 0, 1})
                    ->ID()
                    .str())
        , Bob(client_.Wallet().Nym(reason_, "Bob", {SeedB_, 0, 1})->ID().str())
    {
    }
};

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
TEST_F(Test_CreateNymHD, TestNym_DeterministicIDs)
{

    EXPECT_STREQ(AliceID.c_str(), Alice.c_str());
    EXPECT_STREQ(BobID.c_str(), Bob.c_str());
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32

TEST_F(Test_CreateNymHD, TestNym_ABCD)
{
    const auto NymA = client_.Wallet().Nym(identifier::Nym::Factory(Alice));
    const auto NymB = client_.Wallet().Nym(identifier::Nym::Factory(Bob));
    const auto NymC = client_.Wallet().Nym(reason_, "Charly", {SeedA_, 1});
    const auto NymD = client_.Wallet().Nym(reason_, "Dave", {SeedB_, 1});

    // Alice
    EXPECT_TRUE(NymA->HasPath());
    EXPECT_STREQ(NymA->PathRoot().c_str(), SeedA_.c_str());
    EXPECT_EQ(2, NymA->PathChildSize());

    EXPECT_EQ(
        static_cast<Bip32Index>(Bip43Purpose::NYM) |
            static_cast<Bip32Index>(Bip32Child::HARDENED),
        NymA->PathChild(0));

    EXPECT_EQ(
        0 | static_cast<Bip32Index>(Bip32Child::HARDENED), NymA->PathChild(1));

    // Bob
    EXPECT_TRUE(NymB->HasPath());
    EXPECT_STREQ(NymB->PathRoot().c_str(), SeedB_.c_str());
    EXPECT_EQ(2, NymB->PathChildSize());

    EXPECT_EQ(
        static_cast<Bip32Index>(Bip43Purpose::NYM) |
            static_cast<Bip32Index>(Bip32Child::HARDENED),
        NymB->PathChild(0));

    EXPECT_EQ(
        0 | static_cast<Bip32Index>(Bip32Child::HARDENED), NymB->PathChild(1));

    // Charly
    EXPECT_TRUE(NymC->HasPath());
    EXPECT_STREQ(NymC->PathRoot().c_str(), SeedA_.c_str());
    EXPECT_EQ(2, NymC->PathChildSize());

    EXPECT_EQ(
        static_cast<Bip32Index>(Bip43Purpose::NYM) |
            static_cast<Bip32Index>(Bip32Child::HARDENED),
        NymC->PathChild(0));

    EXPECT_EQ(
        1 | static_cast<Bip32Index>(Bip32Child::HARDENED), NymC->PathChild(1));
}

TEST_F(Test_CreateNymHD, TestNym_Dave)
{
    const auto NymD = client_.Wallet().Nym(reason_, "Dave", {SeedB_, 1});

    ASSERT_TRUE(NymD);

    EXPECT_TRUE(NymD->HasPath());
    EXPECT_STREQ(NymD->PathRoot().c_str(), SeedB_.c_str());
    EXPECT_EQ(2, NymD->PathChildSize());

    EXPECT_EQ(
        static_cast<Bip32Index>(Bip43Purpose::NYM) |
            static_cast<Bip32Index>(Bip32Child::HARDENED),
        NymD->PathChild(0));

    EXPECT_EQ(
        1 | static_cast<Bip32Index>(Bip32Child::HARDENED), NymD->PathChild(1));
}

TEST_F(Test_CreateNymHD, TestNym_Eve)
{
    const auto NymE = client_.Wallet().Nym(reason_, "Eve", {SeedB_, 2, 1});

    ASSERT_TRUE(NymE);

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
    EXPECT_EQ(EveID, NymE->ID().str());
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32

    EXPECT_TRUE(NymE->HasPath());
    EXPECT_STREQ(NymE->PathRoot().c_str(), SeedB_.c_str());
    EXPECT_EQ(2, NymE->PathChildSize());

    EXPECT_EQ(
        static_cast<Bip32Index>(Bip43Purpose::NYM) |
            static_cast<Bip32Index>(Bip32Child::HARDENED),
        NymE->PathChild(0));

    EXPECT_EQ(
        2 | static_cast<Bip32Index>(Bip32Child::HARDENED), NymE->PathChild(1));
}

TEST_F(Test_CreateNymHD, TestNym_Frank)
{
    const auto NymF = client_.Wallet().Nym(reason_, "Frank", {SeedB_, 3, 3});
    const auto NymF2 = client_.Wallet().Nym(reason_, "Frank", {SeedA_, 3, 3});

    EXPECT_NE(NymF->ID(), NymF2->ID());

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
    EXPECT_EQ(FrankID, NymF->ID().str());
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32

    EXPECT_TRUE(NymF->HasPath());
    EXPECT_TRUE(NymF2->HasPath());

    EXPECT_STREQ(NymF->PathRoot().c_str(), SeedB_.c_str());
    EXPECT_STREQ(NymF2->PathRoot().c_str(), SeedA_.c_str());

    EXPECT_EQ(2, NymF->PathChildSize());
    EXPECT_EQ(2, NymF2->PathChildSize());

    EXPECT_EQ(
        static_cast<Bip32Index>(Bip43Purpose::NYM) |
            static_cast<Bip32Index>(Bip32Child::HARDENED),
        NymF->PathChild(0));

    EXPECT_EQ(
        3 | static_cast<Bip32Index>(Bip32Child::HARDENED), NymF->PathChild(1));
    EXPECT_EQ(
        3 | static_cast<Bip32Index>(Bip32Child::HARDENED), NymF2->PathChild(1));
}

TEST_F(Test_CreateNymHD, TestNym_NonnegativeIndex)
{
    const auto Nym1 = client_.Wallet().Nym(reason_, "Nym1", {SeedC_, 0});
    const auto Nym2 = client_.Wallet().Nym(reason_, "Nym2", {SeedC_, 0});

    EXPECT_TRUE(Nym1->HasPath());
    EXPECT_TRUE(Nym2->HasPath());

    const auto nym1Index = Nym1->PathChild(1);
    const auto nym2Index = Nym2->PathChild(1);

    ASSERT_EQ(nym1Index, nym2Index);
}

TEST_F(Test_CreateNymHD, TestNym_NegativeIndex)
{
    const auto Nym1 = client_.Wallet().Nym(reason_, "Nym1", {SeedD_, -1});
    const auto Nym2 = client_.Wallet().Nym(reason_, "Nym2", {SeedD_, -1});

    EXPECT_TRUE(Nym1->HasPath());
    EXPECT_TRUE(Nym2->HasPath());

    const auto nym1Index = Nym1->PathChild(1);
    const auto nym2Index = Nym2->PathChild(1);

    ASSERT_NE(nym1Index, nym2Index);
}
#endif  // OT_CRYPTO_WITH_BIP32
}  // namespace
