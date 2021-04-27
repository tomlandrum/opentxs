// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <utility>

#include "2_Factory.hpp"
#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "internal/api/client/Client.hpp"
#include "internal/blind/Blind.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Editor.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/server/Manager.hpp"
#include "opentxs/blind/Mint.hpp"
#include "opentxs/blind/Purse.hpp"
#include "opentxs/blind/PurseType.hpp"
#include "opentxs/blind/Token.hpp"
#include "opentxs/blind/TokenState.hpp"
#include "opentxs/client/OTAPI_Exec.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/identity/Nym.hpp"

#define MINT_EXPIRE_MONTHS 6
#define MINT_VALID_MONTHS 12
#define REQUEST_PURSE_VALUE 20000

namespace
{
bool init_{false};

class Test_Basic : public ::testing::Test
{
public:
    static ot::OTNymID alice_nym_id_;
    static ot::OTNymID bob_nym_id_;
    static const ot::OTServerID server_id_;
    static const ot::OTUnitID unit_id_;
    static std::shared_ptr<ot::blind::Mint> mint_;
    static std::shared_ptr<ot::blind::Purse> request_purse_;
    static std::shared_ptr<ot::blind::Purse> issue_purse_;
    static ot::Space serialized_bytes_;
    static ot::Time valid_from_;
    static ot::Time valid_to_;

    const ot::api::client::internal::Manager& api_;
    ot::OTPasswordPrompt reason_;
    ot::Nym_p alice_;
    ot::Nym_p bob_;

    Test_Basic()
        : api_(dynamic_cast<const ot::api::client::internal::Manager&>(
              ot::Context().StartClient(OTTestEnvironment::test_args_, 0)))
        , reason_(api_.Factory().PasswordPrompt(__FUNCTION__))
        , alice_()
        , bob_()
    {
        if (false == init_) { init(); }

        alice_ = api_.Wallet().Nym(alice_nym_id_);
        bob_ = api_.Wallet().Nym(bob_nym_id_);
    }

    void init()
    {
        const auto seedA = api_.Exec().Wallet_ImportSeed(
            "spike nominee miss inquiry fee nothing belt list other "
            "daughter leave valley twelve gossip paper",
            "");
        const auto seedB = api_.Exec().Wallet_ImportSeed(
            "trim thunder unveil reduce crop cradle zone inquiry "
            "anchor skate property fringe obey butter text tank drama "
            "palm guilt pudding laundry stay axis prosper",
            "");
        alice_nym_id_ = api_.Wallet().Nym(reason_, "Alice", {seedA, 0})->ID();
        bob_nym_id_ = api_.Wallet().Nym(reason_, "Bob", {seedB, 0})->ID();
        const_cast<ot::identifier::UnitDefinition&>(unit_id_.get())
            .SetString(ot::Identifier::Random()->str());
        const_cast<ot::identifier::Server&>(server_id_.get())
            .SetString(ot::Identifier::Random()->str());
        init_ = true;
    }
};
}  // namespace

ot::OTNymID Test_Basic::alice_nym_id_{ot::identifier::Nym::Factory()};
ot::OTNymID Test_Basic::bob_nym_id_{ot::identifier::Nym::Factory()};
const ot::OTServerID Test_Basic::server_id_{ot::identifier::Server::Factory()};
const ot::OTUnitID Test_Basic::unit_id_{
    ot::identifier::UnitDefinition::Factory()};
std::shared_ptr<ot::blind::Mint> Test_Basic::mint_{};
std::shared_ptr<ot::blind::Purse> Test_Basic::request_purse_{};
std::shared_ptr<ot::blind::Purse> Test_Basic::issue_purse_{};
ot::Space Test_Basic::serialized_bytes_{};
ot::Time Test_Basic::valid_from_;
ot::Time Test_Basic::valid_to_;

TEST_F(Test_Basic, generateMint)
{
    mint_.reset(api_.Factory()
                    .Mint(
                        ot::String::Factory(bob_nym_id_->str()),
                        ot::String::Factory(unit_id_->str()))
                    .release());

    ASSERT_TRUE(mint_);

    const auto& nym = *bob_;
    const auto now = ot::Clock::now();
    const std::chrono::seconds expireInterval(
        std::chrono::hours(MINT_EXPIRE_MONTHS * 30 * 24));
    const std::chrono::seconds validInterval(
        std::chrono::hours(MINT_VALID_MONTHS * 30 * 24));
    const auto expires = now + expireInterval;
    const auto validTo = now + validInterval;
    valid_from_ = now;
    valid_to_ = validTo;
    mint_->GenerateNewMint(
        api_.Wallet(),
        0,
        now,
        validTo,
        expires,
        unit_id_,
        server_id_,
        nym,
        1,
        10,
        100,
        1000,
        10000,
        100000,
        1000000,
        10000000,
        100000000,
        1000000000,
        OT_MINT_KEY_SIZE_TEST,
        reason_);
}

TEST_F(Test_Basic, requestPurse)
{
    ASSERT_TRUE(alice_);
    ASSERT_TRUE(bob_);
    ASSERT_TRUE(mint_);

    request_purse_.reset(ot::Factory::Purse(
        api_,
        *alice_,
        server_id_,
        *bob_,
        ot::blind::CashType::Lucre,
        *mint_,
        REQUEST_PURSE_VALUE,
        reason_));

    ASSERT_TRUE(request_purse_);

    auto& purse = *request_purse_;

    ASSERT_TRUE(purse.IsUnlocked());
    EXPECT_EQ(
        ot::Clock::to_time_t(purse.EarliestValidTo()),
        ot::Clock::to_time_t(valid_to_));
    EXPECT_EQ(
        ot::Clock::to_time_t(purse.LatestValidFrom()),
        ot::Clock::to_time_t(valid_from_));
    EXPECT_EQ(server_id_, purse.Notary());
    EXPECT_EQ(purse.State(), ot::blind::PurseType::Request);
    EXPECT_EQ(purse.Type(), ot::blind::CashType::Lucre);
    EXPECT_EQ(unit_id_, purse.Unit());
    EXPECT_EQ(purse.Value(), REQUEST_PURSE_VALUE);
    ASSERT_EQ(purse.size(), 2);

    auto& token1 = purse.at(0);

    EXPECT_EQ(server_id_, token1.Notary());
    EXPECT_EQ(token1.Series(), 0);
    EXPECT_EQ(token1.State(), ot::blind::TokenState::Blinded);
    EXPECT_EQ(token1.Type(), ot::blind::CashType::Lucre);
    EXPECT_EQ(unit_id_, token1.Unit());
    EXPECT_EQ(
        ot::Clock::to_time_t(token1.ValidFrom()),
        ot::Clock::to_time_t(valid_from_));
    EXPECT_EQ(
        ot::Clock::to_time_t(token1.ValidTo()),
        ot::Clock::to_time_t(valid_to_));
    EXPECT_EQ(token1.Value(), 10000);

    auto& token2 = purse.at(1);

    EXPECT_EQ(server_id_, token2.Notary());
    EXPECT_EQ(token2.Series(), 0);
    EXPECT_EQ(token2.State(), ot::blind::TokenState::Blinded);
    EXPECT_EQ(token2.Type(), ot::blind::CashType::Lucre);
    EXPECT_EQ(unit_id_, token2.Unit());
    EXPECT_EQ(
        ot::Clock::to_time_t(token2.ValidFrom()),
        ot::Clock::to_time_t(valid_from_));
    EXPECT_EQ(
        ot::Clock::to_time_t(token2.ValidTo()),
        ot::Clock::to_time_t(valid_to_));
    EXPECT_EQ(token2.Value(), 10000);
}

TEST_F(Test_Basic, serialize_deserialize)
{
    ASSERT_TRUE(request_purse_);

    request_purse_->Serialize(opentxs::writer(serialized_bytes_));

    std::unique_ptr<ot::blind::Purse> restored{
        ot::Factory::Purse(api_, ot::reader(serialized_bytes_))};

    ASSERT_TRUE(restored);

    EXPECT_EQ(
        ot::Clock::to_time_t(request_purse_->EarliestValidTo()),
        ot::Clock::to_time_t(restored->EarliestValidTo()));
    EXPECT_EQ(
        ot::Clock::to_time_t(request_purse_->LatestValidFrom()),
        ot::Clock::to_time_t(restored->LatestValidFrom()));
    EXPECT_EQ(request_purse_->Notary(), restored->Notary());
    EXPECT_EQ(request_purse_->State(), restored->State());
    EXPECT_EQ(request_purse_->Type(), restored->Type());
    EXPECT_EQ(request_purse_->Unit(), restored->Unit());
    EXPECT_EQ(request_purse_->Value(), restored->Value());

    EXPECT_EQ(2, restored->size());

    for (std::size_t i = 0; i < restored->size(); ++i) {
        auto& token_a = request_purse_->at(i);
        auto& token_b = restored->at(i);

        EXPECT_EQ(token_a.Notary(), token_b.Notary());
        EXPECT_EQ(token_a.Series(), token_b.Series());
        EXPECT_EQ(token_a.State(), token_b.State());
        EXPECT_EQ(token_a.Type(), token_b.Type());
        EXPECT_EQ(token_a.Unit(), token_b.Unit());
        EXPECT_EQ(
            ot::Clock::to_time_t(token_a.ValidFrom()),
            ot::Clock::to_time_t(token_b.ValidFrom()));
        EXPECT_EQ(
            ot::Clock::to_time_t(token_a.ValidTo()),
            ot::Clock::to_time_t(token_b.ValidTo()));
        EXPECT_EQ(token_a.Value(), token_b.Value());
    }
}

TEST_F(Test_Basic, sign)
{
    std::unique_ptr<ot::blind::Purse> restored{
        ot::Factory::Purse(api_, ot::reader(serialized_bytes_))};

    ASSERT_TRUE(restored);

    ASSERT_TRUE(mint_);
    ASSERT_TRUE(alice_);
    ASSERT_TRUE(bob_);

    auto& requestPurse = *restored;
    auto& alice = *alice_;
    auto& bob = *bob_;

    EXPECT_TRUE(requestPurse.Unlock(bob, reason_));
    ASSERT_TRUE(requestPurse.IsUnlocked());

    issue_purse_.reset(ot::Factory::Purse(api_, requestPurse, alice, reason_));

    ASSERT_TRUE(issue_purse_);

    auto& issuePurse = *issue_purse_;

    EXPECT_TRUE(issuePurse.IsUnlocked());

    auto& mint = *mint_;
    auto pToken = requestPurse.Pop();
    const auto added = issuePurse.AddNym(bob, reason_);

    EXPECT_TRUE(added);

    while (pToken) {
        auto& token = *pToken;
        const auto signature = mint.SignToken(bob, token, reason_);

        EXPECT_TRUE(signature);
        EXPECT_TRUE(ot::blind::TokenState::Signed == token.State());

        const auto push = issuePurse.Push(pToken, reason_);

        EXPECT_TRUE(push);

        pToken = requestPurse.Pop();
    }

    EXPECT_TRUE(ot::blind::PurseType::Issue == issuePurse.State());
    EXPECT_EQ(issuePurse.Notary().str(), requestPurse.Notary().str());
    EXPECT_EQ(issuePurse.Type(), requestPurse.Type());
    EXPECT_EQ(issuePurse.Unit().str(), requestPurse.Unit().str());
    EXPECT_EQ(issuePurse.Value(), REQUEST_PURSE_VALUE);
    EXPECT_EQ(requestPurse.Value(), 0);
}

TEST_F(Test_Basic, process)
{
    ASSERT_TRUE(issue_purse_);
    ASSERT_TRUE(mint_);
    ASSERT_TRUE(alice_);

    auto& issuePurse = *issue_purse_;

    auto bytes = ot::Space{};
    issuePurse.Serialize(opentxs::writer(bytes));
    std::unique_ptr<ot::blind::Purse> restored{
        ot::Factory::Purse(api_, ot::reader(bytes))};

    ASSERT_TRUE(restored);

    auto& purse = *restored;
    auto& mint = *mint_;
    auto& alice = *alice_;

    EXPECT_TRUE(purse.Unlock(alice, reason_));
    ASSERT_TRUE(purse.IsUnlocked());
    EXPECT_TRUE(purse.Process(alice, mint, reason_));
    EXPECT_TRUE(ot::blind::PurseType::Normal == purse.State());

    issue_purse_ = std::move(restored);
}

TEST_F(Test_Basic, verify)
{
    ASSERT_TRUE(issue_purse_);
    ASSERT_TRUE(mint_);
    ASSERT_TRUE(bob_);

    auto& issuePurse = *issue_purse_;
    auto& bob = *bob_;

    EXPECT_TRUE(issuePurse.Unlock(bob, reason_));
    ASSERT_TRUE(issuePurse.IsUnlocked());

    auto bytes = ot::Space{};
    issuePurse.Serialize(opentxs::writer(bytes));
    std::unique_ptr<ot::blind::Purse> restored{
        ot::Factory::Purse(api_, ot::reader(bytes))};

    ASSERT_TRUE(restored);

    auto& purse = *restored;

    EXPECT_TRUE(purse.Unlock(bob, reason_));
    ASSERT_TRUE(purse.IsUnlocked());

    auto& mint = *mint_;

    for (const auto& token : purse) {
        EXPECT_FALSE(token.IsSpent(reason_));

        const auto verified = mint.VerifyToken(bob, token, reason_);

        EXPECT_TRUE(verified);
    }

    issue_purse_ = std::move(restored);
}

TEST_F(Test_Basic, wallet)
{
    {
        auto purse =
            api_.Wallet().Purse(alice_nym_id_, server_id_, unit_id_, true);

        EXPECT_FALSE(purse);
    }

    {
        api_.Wallet().mutable_Purse(
            alice_nym_id_,
            server_id_,
            unit_id_,
            reason_,
            ot::blind::CashType::Lucre);
    }

    {
        auto purse =
            api_.Wallet().Purse(alice_nym_id_, server_id_, unit_id_, false);

        EXPECT_TRUE(purse);
    }
}

TEST_F(Test_Basic, PushPop)
{
    auto purseEditor = api_.Wallet().mutable_Purse(
        alice_nym_id_,
        server_id_,
        unit_id_,
        reason_,
        ot::blind::CashType::Lucre);
    auto& purse = purseEditor.get();

    ASSERT_TRUE(issue_purse_);

    auto& issuePurse = *issue_purse_;
    auto& alice = *alice_;
    const auto unlocked = issuePurse.Unlock(alice, reason_);

    ASSERT_TRUE(unlocked);
    ASSERT_TRUE(purse.Unlock(alice, reason_));
    ASSERT_TRUE(purse.IsUnlocked());

    auto token = issuePurse.Pop();

    while (token) {
        EXPECT_TRUE(token->MarkSpent(reason_));
        EXPECT_TRUE(token->IsSpent(reason_));
        EXPECT_TRUE(purse.Push(token, reason_));

        token = issuePurse.Pop();
    }

    EXPECT_EQ(purse.size(), 2);
    EXPECT_EQ(purse.Value(), 0);
    EXPECT_EQ(issuePurse.Value(), 0);
}
