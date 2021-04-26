// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <future>
#include <memory>
#include <string>
#include <utility>

#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Shared.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/client/OTX.hpp"
#include "opentxs/api/server/Manager.hpp"
#include "opentxs/client/NymData.hpp"
#include "opentxs/client/OTAPI_Exec.hpp"
#include "opentxs/contact/Contact.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Message.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/core/contract/UnitType.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/otx/LastReplyStatus.hpp"

using namespace opentxs;

#define ALEX "Alice"
#define BOB "Bob"
#define ISSUER "Issuer"

#define UNIT_DEFINITION_CONTRACT_NAME "Mt Gox USD"
#define UNIT_DEFINITION_TERMS "YOLO"
#define UNIT_DEFINITION_PRIMARY_UNIT_NAME "dollars"
#define UNIT_DEFINITION_SYMBOL "$"
#define UNIT_DEFINITION_TLA "USD"
#define UNIT_DEFINITION_POWER 2
#define UNIT_DEFINITION_FRACTIONAL_UNIT_NAME "cents"
#define UNIT_DEFINITION_UNIT_OF_ACCOUNT ot::contact::ContactItemType::USD
#define CHEQUE_AMOUNT_1 2000
#define CHEQUE_MEMO_1 "memo"

// #define OT_METHOD "::Test_DepositCheques::"

namespace
{
bool init_{false};

class Test_DepositCheques : public ::testing::Test
{
public:
    static const std::string SeedA_;
    static const std::string SeedB_;
    static const std::string SeedC_;
    static const OTNymID alice_nym_id_;
    static const OTNymID bob_nym_id_;
    static const OTNymID issuer_nym_id_;
    static OTIdentifier contact_id_alice_bob_;
    static OTIdentifier contact_id_alice_issuer_;
    static OTIdentifier contact_id_bob_alice_;
    static OTIdentifier contact_id_bob_issuer_;
    static OTIdentifier contact_id_issuer_alice_;
    static OTIdentifier contact_id_issuer_bob_;

    static const ot::api::client::Manager* alice_;
    static const ot::api::client::Manager* bob_;

    static std::string alice_payment_code_;
    static std::string bob_payment_code_;
    static std::string issuer_payment_code_;

    static OTUnitID unit_id_;
    static OTIdentifier alice_account_id_;
    static OTIdentifier issuer_account_id_;

    const ot::api::client::Manager& alice_client_;
    const ot::api::client::Manager& bob_client_;
    const ot::api::server::Manager& server_1_;
    const ot::api::client::Manager& issuer_client_;
    const OTServerContract server_contract_;

    Test_DepositCheques()
        : alice_client_(Context().StartClient(OTTestEnvironment::test_args_, 0))
        , bob_client_(Context().StartClient(OTTestEnvironment::test_args_, 1))
        , server_1_(
              Context().StartServer(OTTestEnvironment::test_args_, 0, true))
        , issuer_client_(
              Context().StartClient(OTTestEnvironment::test_args_, 2))
        , server_contract_(server_1_.Wallet().Server(server_1_.ID()))
    {
#if OT_CASH
        server_1_.SetMintKeySize(OT_MINT_KEY_SIZE_TEST);
#endif

        if (false == init_) { init(); }
    }

    void import_server_contract(
        const contract::Server& contract,
        const ot::api::client::Manager& client)
    {
        auto reason = client.Factory().PasswordPrompt(__FUNCTION__);
        auto bytes = ot::Space{};
        EXPECT_TRUE(server_contract_->PublicContractToBytes(ot::writer(bytes)));
        auto clientVersion = client.Wallet().Server(bytes);

        client.OTX().SetIntroductionServer(clientVersion);
    }

    void init()
    {
        const_cast<std::string&>(SeedA_) =
            alice_client_.Exec().Wallet_ImportSeed(
                "spike nominee miss inquiry fee nothing belt list other "
                "daughter leave valley twelve gossip paper",
                "");
        const_cast<std::string&>(SeedB_) = bob_client_.Exec().Wallet_ImportSeed(
            "trim thunder unveil reduce crop cradle zone inquiry "
            "anchor skate property fringe obey butter text tank drama "
            "palm guilt pudding laundry stay axis prosper",
            "");
        const_cast<std::string&>(SeedC_) =
            issuer_client_.Exec().Wallet_ImportSeed(
                "abandon abandon abandon abandon abandon abandon abandon "
                "abandon abandon abandon abandon about",
                "");
        auto reasonA = alice_client_.Factory().PasswordPrompt(__FUNCTION__);
        auto reasonB = bob_client_.Factory().PasswordPrompt(__FUNCTION__);
        auto reasonI = issuer_client_.Factory().PasswordPrompt(__FUNCTION__);
        const_cast<OTNymID&>(alice_nym_id_) =
            alice_client_.Wallet().Nym(reasonA, ALEX, {SeedA_, 0})->ID();
        const_cast<OTNymID&>(bob_nym_id_) =
            bob_client_.Wallet().Nym(reasonB, BOB, {SeedB_, 0})->ID();
        const_cast<OTNymID&>(issuer_nym_id_) =
            issuer_client_.Wallet().Nym(reasonI, ISSUER, {SeedC_, 0})->ID();

        import_server_contract(server_contract_, alice_client_);
        import_server_contract(server_contract_, bob_client_);
        import_server_contract(server_contract_, issuer_client_);

        alice_ = &alice_client_;
        bob_ = &bob_client_;

        init_ = true;
    }
};

const std::string Test_DepositCheques::SeedA_{""};
const std::string Test_DepositCheques::SeedB_{""};
const std::string Test_DepositCheques::SeedC_{""};
const OTNymID Test_DepositCheques::alice_nym_id_{identifier::Nym::Factory()};
const OTNymID Test_DepositCheques::bob_nym_id_{identifier::Nym::Factory()};
const OTNymID Test_DepositCheques::issuer_nym_id_{identifier::Nym::Factory()};
OTIdentifier Test_DepositCheques::contact_id_alice_bob_{Identifier::Factory()};
OTIdentifier Test_DepositCheques::contact_id_alice_issuer_{
    Identifier::Factory()};
OTIdentifier Test_DepositCheques::contact_id_bob_alice_{Identifier::Factory()};
OTIdentifier Test_DepositCheques::contact_id_bob_issuer_{Identifier::Factory()};
OTIdentifier Test_DepositCheques::contact_id_issuer_alice_{
    Identifier::Factory()};
OTIdentifier Test_DepositCheques::contact_id_issuer_bob_{Identifier::Factory()};
const ot::api::client::Manager* Test_DepositCheques::alice_{nullptr};
const ot::api::client::Manager* Test_DepositCheques::bob_{nullptr};
std::string Test_DepositCheques::alice_payment_code_;
std::string Test_DepositCheques::bob_payment_code_;
std::string Test_DepositCheques::issuer_payment_code_;
OTUnitID Test_DepositCheques::unit_id_{identifier::UnitDefinition::Factory()};
OTIdentifier Test_DepositCheques::alice_account_id_{Identifier::Factory()};
OTIdentifier Test_DepositCheques::issuer_account_id_{Identifier::Factory()};

TEST_F(Test_DepositCheques, payment_codes)
{
    auto reasonA = alice_client_.Factory().PasswordPrompt(__FUNCTION__);
    auto reasonB = bob_client_.Factory().PasswordPrompt(__FUNCTION__);
    auto reasonI = issuer_client_.Factory().PasswordPrompt(__FUNCTION__);
    auto alice = alice_client_.Wallet().mutable_Nym(alice_nym_id_, reasonA);
    auto bob = bob_client_.Wallet().mutable_Nym(bob_nym_id_, reasonB);
    auto issuer = issuer_client_.Wallet().mutable_Nym(issuer_nym_id_, reasonI);

    EXPECT_EQ(ot::contact::ContactItemType::Individual, alice.Type());
    EXPECT_EQ(ot::contact::ContactItemType::Individual, bob.Type());
    EXPECT_EQ(ot::contact::ContactItemType::Individual, issuer.Type());

    auto aliceScopeSet = alice.SetScope(
        ot::contact::ContactItemType::Individual, ALEX, true, reasonA);
    auto bobScopeSet =
        bob.SetScope(contact::ContactItemType::Individual, BOB, true, reasonB);
    auto issuerScopeSet = issuer.SetScope(
        contact::ContactItemType::Individual, ISSUER, true, reasonI);

    EXPECT_TRUE(aliceScopeSet);
    EXPECT_TRUE(bobScopeSet);
    EXPECT_TRUE(issuerScopeSet);

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
    alice_payment_code_ =
        alice_client_.Factory().PaymentCode(SeedA_, 0, 1, reasonA)->asBase58();
    bob_payment_code_ =
        bob_client_.Factory().PaymentCode(SeedB_, 0, 1, reasonB)->asBase58();
    issuer_payment_code_ =
        issuer_client_.Factory().PaymentCode(SeedC_, 0, 1, reasonI)->asBase58();

    EXPECT_FALSE(alice_payment_code_.empty());
    EXPECT_FALSE(bob_payment_code_.empty());
    EXPECT_FALSE(issuer_payment_code_.empty());

    alice.AddPaymentCode(
        alice_payment_code_,
        ot::contact::ContactItemType::BTC,
        true,
        true,
        reasonA);
    bob.AddPaymentCode(
        bob_payment_code_,
        ot::contact::ContactItemType::BTC,
        true,
        true,
        reasonB);
    issuer.AddPaymentCode(
        issuer_payment_code_,
        ot::contact::ContactItemType::BTC,
        true,
        true,
        reasonI);
    alice.AddPaymentCode(
        alice_payment_code_,
        ot::contact::ContactItemType::BCH,
        true,
        true,
        reasonA);
    bob.AddPaymentCode(
        bob_payment_code_,
        ot::contact::ContactItemType::BCH,
        true,
        true,
        reasonB);
    issuer.AddPaymentCode(
        issuer_payment_code_,
        ot::contact::ContactItemType::BCH,
        true,
        true,
        reasonI);

    EXPECT_FALSE(alice.PaymentCode(contact::ContactItemType::BTC).empty());
    EXPECT_FALSE(bob.PaymentCode(contact::ContactItemType::BTC).empty());
    EXPECT_FALSE(issuer.PaymentCode(contact::ContactItemType::BTC).empty());
    EXPECT_FALSE(alice.PaymentCode(contact::ContactItemType::BCH).empty());
    EXPECT_FALSE(bob.PaymentCode(contact::ContactItemType::BCH).empty());
    EXPECT_FALSE(issuer.PaymentCode(contact::ContactItemType::BCH).empty());
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32

    alice.Release();
    bob.Release();
    issuer.Release();
}

TEST_F(Test_DepositCheques, introduction_server)
{
    alice_client_.OTX().StartIntroductionServer(alice_nym_id_);
    bob_client_.OTX().StartIntroductionServer(bob_nym_id_);
    auto task1 = alice_client_.OTX().RegisterNymPublic(
        alice_nym_id_, server_1_.ID(), true);
    auto task2 =
        bob_client_.OTX().RegisterNymPublic(bob_nym_id_, server_1_.ID(), true);

    ASSERT_NE(0, task1.first);
    ASSERT_NE(0, task2.first);
    EXPECT_EQ(otx::LastReplyStatus::MessageSuccess, task1.second.get().first);
    EXPECT_EQ(otx::LastReplyStatus::MessageSuccess, task2.second.get().first);

    alice_client_.OTX().ContextIdle(alice_nym_id_, server_1_.ID()).get();
    bob_client_.OTX().ContextIdle(bob_nym_id_, server_1_.ID()).get();
}

TEST_F(Test_DepositCheques, add_contacts)
{
    const auto aliceBob = alice_client_.Contacts().NewContact(
        BOB,
        bob_nym_id_,
        alice_client_.Factory().PaymentCode(bob_payment_code_));
    const auto aliceIssuer = alice_client_.Contacts().NewContact(
        ISSUER,
        issuer_nym_id_,
        alice_client_.Factory().PaymentCode(issuer_payment_code_));
    const auto bobAlice = bob_client_.Contacts().NewContact(
        ALEX,
        alice_nym_id_,
        bob_client_.Factory().PaymentCode(alice_payment_code_));
    const auto bobIssuer = bob_client_.Contacts().NewContact(
        ISSUER,
        issuer_nym_id_,
        bob_client_.Factory().PaymentCode(issuer_payment_code_));
    const auto issuerAlice = issuer_client_.Contacts().NewContact(
        ALEX,
        alice_nym_id_,
        issuer_client_.Factory().PaymentCode(alice_payment_code_));
    const auto issuerBob = issuer_client_.Contacts().NewContact(
        BOB,
        bob_nym_id_,
        issuer_client_.Factory().PaymentCode(bob_payment_code_));

    ASSERT_TRUE(aliceBob);
    ASSERT_TRUE(aliceIssuer);
    ASSERT_TRUE(bobAlice);
    ASSERT_TRUE(bobIssuer);
    ASSERT_TRUE(issuerAlice);
    ASSERT_TRUE(issuerBob);

    contact_id_alice_bob_ = aliceBob->ID();
    contact_id_alice_issuer_ = aliceIssuer->ID();
    contact_id_bob_alice_ = bobAlice->ID();
    contact_id_bob_issuer_ = bobIssuer->ID();
    contact_id_issuer_alice_ = issuerAlice->ID();
    contact_id_issuer_bob_ = issuerAlice->ID();

    auto bytes = ot::Space{};
    bob_client_.Wallet().Nym(bob_nym_id_)->asPublicNym(ot::writer(bytes));
    EXPECT_TRUE(alice_client_.Wallet().Nym(bytes));

    issuer_client_.Wallet().Nym(issuer_nym_id_)->asPublicNym(ot::writer(bytes));
    EXPECT_TRUE(alice_client_.Wallet().Nym(bytes));
    alice_client_.Wallet().Nym(alice_nym_id_)->asPublicNym(ot::writer(bytes));
    EXPECT_TRUE(bob_client_.Wallet().Nym(bytes));
    issuer_client_.Wallet().Nym(issuer_nym_id_)->asPublicNym(ot::writer(bytes));
    EXPECT_TRUE(bob_client_.Wallet().Nym(bytes));
    alice_client_.Wallet().Nym(alice_nym_id_)->asPublicNym(ot::writer(bytes));
    EXPECT_TRUE(issuer_client_.Wallet().Nym(bytes));
    bob_client_.Wallet().Nym(bob_nym_id_)->asPublicNym(ot::writer(bytes));
    EXPECT_TRUE(issuer_client_.Wallet().Nym(bytes));
}

TEST_F(Test_DepositCheques, issue_dollars)
{
    auto reasonI = issuer_client_.Factory().PasswordPrompt(__FUNCTION__);
    const auto contract = issuer_client_.Wallet().UnitDefinition(
        issuer_nym_id_->str(),
        UNIT_DEFINITION_CONTRACT_NAME,
        UNIT_DEFINITION_TERMS,
        UNIT_DEFINITION_PRIMARY_UNIT_NAME,
        UNIT_DEFINITION_SYMBOL,
        UNIT_DEFINITION_TLA,
        UNIT_DEFINITION_POWER,
        UNIT_DEFINITION_FRACTIONAL_UNIT_NAME,
        UNIT_DEFINITION_UNIT_OF_ACCOUNT,
        reasonI);

    EXPECT_EQ(contract::UnitType::Currency, contract->Type());
    EXPECT_TRUE(unit_id_->empty());

    unit_id_->Assign(contract->ID());

    EXPECT_FALSE(unit_id_->empty());

    {
        auto issuer =
            issuer_client_.Wallet().mutable_Nym(issuer_nym_id_, reasonI);
        issuer.AddPreferredOTServer(server_1_.ID().str(), true, reasonI);
        issuer.AddContract(
            unit_id_->str(),
            contact::ContactItemType::USD,
            true,
            true,
            reasonI);
    }

    auto task = issuer_client_.OTX().IssueUnitDefinition(
        issuer_nym_id_, server_1_.ID(), unit_id_);
    auto& [taskID, future] = task;
    const auto result = future.get();

    EXPECT_NE(0, taskID);
    EXPECT_EQ(otx::LastReplyStatus::MessageSuccess, result.first);
    ASSERT_TRUE(result.second);

    issuer_account_id_->SetString(result.second->m_strAcctID);

    EXPECT_FALSE(issuer_account_id_->empty());

    issuer_client_.OTX().ContextIdle(issuer_nym_id_, server_1_.ID()).get();
}

TEST_F(Test_DepositCheques, pay_alice)
{
    auto task = issuer_client_.OTX().SendCheque(
        issuer_nym_id_,
        issuer_account_id_,
        contact_id_issuer_alice_,
        CHEQUE_AMOUNT_1,
        CHEQUE_MEMO_1);
    auto& [taskID, future] = task;

    ASSERT_NE(0, taskID);
    EXPECT_EQ(otx::LastReplyStatus::MessageSuccess, future.get().first);

    issuer_client_.OTX().ContextIdle(issuer_nym_id_, server_1_.ID()).get();
    alice_client_.OTX().ContextIdle(alice_nym_id_, server_1_.ID()).get();
}

TEST_F(Test_DepositCheques, accept_cheque_alice)
{
    // No meaning to this operation other than to ensure the state machine has
    // completed one full cycle
    alice_client_.OTX()
        .DownloadServerContract(alice_nym_id_, server_1_.ID(), server_1_.ID())
        .second.get();
    alice_client_.OTX().ContextIdle(alice_nym_id_, server_1_.ID()).get();
    const auto count = alice_client_.OTX().DepositCheques(alice_nym_id_);

    EXPECT_EQ(1, count);

    alice_client_.OTX().ContextIdle(alice_nym_id_, server_1_.ID()).get();
    issuer_client_.OTX().ContextIdle(issuer_nym_id_, server_1_.ID()).get();
}

TEST_F(Test_DepositCheques, process_inbox_issuer)
{
    auto task = issuer_client_.OTX().ProcessInbox(
        issuer_nym_id_, server_1_.ID(), issuer_account_id_);
    auto& [id, future] = task;

    ASSERT_NE(0, id);

    const auto [status, message] = future.get();

    EXPECT_EQ(otx::LastReplyStatus::MessageSuccess, status);
    ASSERT_TRUE(message);

    const auto account = issuer_client_.Wallet().Account(issuer_account_id_);

    EXPECT_EQ(-1 * CHEQUE_AMOUNT_1, account.get().GetBalance());
}

TEST_F(Test_DepositCheques, shutdown)
{
    alice_client_.OTX().ContextIdle(alice_nym_id_, server_1_.ID()).get();
    bob_client_.OTX().ContextIdle(bob_nym_id_, server_1_.ID()).get();
    issuer_client_.OTX().ContextIdle(issuer_nym_id_, server_1_.ID()).get();
}
}  // namespace
