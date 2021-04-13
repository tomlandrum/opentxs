// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Helpers.hpp"  // IWYU pragma: associated

#include <gtest/gtest.h>
#include <deque>
#include <optional>

#include "UIHelpers.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/api/HDSeed.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/client/UI.hpp"
#include "opentxs/api/client/blockchain/AddressStyle.hpp"
#include "opentxs/api/client/blockchain/BalanceList.hpp"
#include "opentxs/api/client/blockchain/BalanceNode.hpp"
#include "opentxs/api/client/blockchain/BalanceNodeType.hpp"
#include "opentxs/api/client/blockchain/BalanceTree.hpp"
#include "opentxs/api/client/blockchain/HD.hpp"
#include "opentxs/api/client/blockchain/PaymentCode.hpp"
#include "opentxs/api/client/blockchain/Subchain.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/blockchain/block/bitcoin/Input.hpp"  // IWYU pragma: keep
#include "opentxs/blockchain/block/bitcoin/Inputs.hpp"
#include "opentxs/blockchain/block/bitcoin/Outputs.hpp"
#include "opentxs/blockchain/block/bitcoin/Script.hpp"  // IWYU pragma: keep
#include "opentxs/blockchain/block/bitcoin/Transaction.hpp"
#include "opentxs/blockchain/client/BlockOracle.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/crypto/Language.hpp"
#include "opentxs/crypto/SeedStyle.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/ui/AccountActivity.hpp"
#include "opentxs/ui/BalanceItem.hpp"
#include "paymentcode/VectorsV3.hpp"

constexpr auto blocks_ = std::uint64_t{200u};
constexpr auto tx_per_block_ = std::uint64_t{500u};
constexpr auto transaction_count_ = blocks_ * tx_per_block_;
constexpr auto amount_ = std::uint64_t{100000000u};
bool first_block_{true};
Counter account_activity_{};

class Regtest_stress : public Regtest_fixture_normal
{
protected:
    using Subchain = ot::api::client::blockchain::Subchain;
    using Transactions = std::deque<ot::blockchain::block::pTxid>;

    static ot::Nym_p alice_p_;
    static ot::Nym_p bob_p_;
    static Transactions transactions_;
    static std::unique_ptr<ScanListener> listener_alice_p_;
    static std::unique_ptr<ScanListener> listener_bob_p_;

    const ot::identity::Nym& alice_;
    const ot::identity::Nym& bob_;
    const ot::api::client::blockchain::HD& alice_account_;
    const ot::api::client::blockchain::HD& bob_account_;
    const ot::Identifier& expected_account_;
    const ot::identifier::Server& expected_notary_;
    const ot::identifier::UnitDefinition& expected_unit_;
    const std::string expected_display_unit_;
    const std::string expected_account_name_;
    const std::string expected_notary_name_;
    const std::string memo_outgoing_;
    const ot::AccountType expected_account_type_;
    const ot::contact::ContactItemType expected_unit_type_;
    const Generator mine_to_alice_;
    const GetBytes get_p2pk_bytes_;
    const GetPattern get_p2pk_patterns_;
    ScanListener& listener_alice_;
    ScanListener& listener_bob_;

    auto GetAddresses() noexcept -> std::vector<std::string>
    {
        auto output = std::vector<std::string>{};
        output.reserve(tx_per_block_);
        const auto reason = client_2_.Factory().PasswordPrompt(__FUNCTION__);
        const auto& bob = client_2_.Blockchain()
                              .Account(bob_.ID(), test_chain_)
                              .GetHD()
                              .at(0);
        const auto indices =
            bob.Reserve(Subchain::External, tx_per_block_, reason);

        OT_ASSERT(indices.size() == tx_per_block_);

        for (const auto index : indices) {
            const auto& element = bob.BalanceElement(Subchain::External, index);
            using Style = ot::api::client::blockchain::AddressStyle;
            const auto& address =
                output.emplace_back(element.Address(Style::P2PKH));

            OT_ASSERT(false == address.empty());
        }

        return output;
    }

    auto Shutdown() noexcept -> void final
    {
        listener_bob_p_.reset();
        listener_alice_p_.reset();
        transactions_.clear();
        bob_p_.reset();
        alice_p_.reset();
        Regtest_fixture_normal::Shutdown();
    }

    Regtest_stress()
        : Regtest_fixture_normal(2)
        , alice_([&]() -> const ot::identity::Nym& {
            if (!alice_p_) {
                const auto reason =
                    client_1_.Factory().PasswordPrompt(__FUNCTION__);
                const auto& vector = vectors_3_.alice_;
                const auto seedID = [&] {
                    const auto words =
                        client_1_.Factory().SecretFromText(vector.words_);
                    const auto phrase = client_1_.Factory().Secret(0);

                    return client_1_.Seeds().ImportSeed(
                        words,
                        phrase,
                        ot::crypto::SeedStyle::BIP39,
                        ot::crypto::Language::en,
                        reason);
                }();

                alice_p_ = client_1_.Wallet().Nym(reason, "Alice", {seedID, 0});

                OT_ASSERT(alice_p_)
                OT_ASSERT(alice_p_->PaymentCode() == vector.payment_code_)

                client_1_.Blockchain().NewHDSubaccount(
                    alice_p_->ID(),
                    ot::BlockchainAccountType::BIP44,
                    test_chain_,
                    reason);
            }

            OT_ASSERT(alice_p_)

            return *alice_p_;
        }())
        , bob_([&]() -> const ot::identity::Nym& {
            if (!bob_p_) {
                const auto reason =
                    client_2_.Factory().PasswordPrompt(__FUNCTION__);
                const auto& vector = vectors_3_.bob_;
                const auto seedID = [&] {
                    const auto words =
                        client_2_.Factory().SecretFromText(vector.words_);
                    const auto phrase = client_2_.Factory().Secret(0);

                    return client_2_.Seeds().ImportSeed(
                        words,
                        phrase,
                        ot::crypto::SeedStyle::BIP39,
                        ot::crypto::Language::en,
                        reason);
                }();

                bob_p_ = client_2_.Wallet().Nym(reason, "Alice", {seedID, 0});

                OT_ASSERT(bob_p_)

                client_2_.Blockchain().NewHDSubaccount(
                    bob_p_->ID(),
                    ot::BlockchainAccountType::BIP44,
                    test_chain_,
                    reason);
            }

            OT_ASSERT(bob_p_)

            return *bob_p_;
        }())
        , alice_account_(client_1_.Blockchain()
                             .Account(alice_.ID(), test_chain_)
                             .GetHD()
                             .at(0))
        , bob_account_(client_2_.Blockchain()
                           .Account(bob_.ID(), test_chain_)
                           .GetHD()
                           .at(0))
        , expected_account_(client_1_.UI().BlockchainAccountID(test_chain_))
        , expected_notary_(client_1_.UI().BlockchainNotaryID(test_chain_))
        , expected_unit_(client_1_.UI().BlockchainUnitID(test_chain_))
        , expected_display_unit_(u8"UNITTEST")
        , expected_account_name_(u8"This device")
        , expected_notary_name_(u8"Unit Test Simulation")
        , memo_outgoing_("memo for outgoing transaction")
        , expected_account_type_(ot::AccountType::Blockchain)
        , expected_unit_type_(ot::contact::ContactItemType::Regtest)
        , mine_to_alice_([&](Height height) -> Transaction {
            using OutputBuilder = ot::api::Factory::OutputBuilder;

            auto output = miner_.Factory().BitcoinGenerationTransaction(
                test_chain_,
                height,
                [&] {
                    namespace c = std::chrono;
                    auto output = std::vector<OutputBuilder>{};
                    const auto reason =
                        client_1_.Factory().PasswordPrompt(__FUNCTION__);
                    const auto keys =
                        std::set<ot::api::client::blockchain::Key>{};
                    const auto target = [] {
                        if (first_block_) {
                            first_block_ = false;

                            return tx_per_block_ * 2u;
                        } else {

                            return tx_per_block_;
                        }
                    }();
                    const auto indices = alice_account_.Reserve(
                        Subchain::External, target, reason);

                    OT_ASSERT(indices.size() == target);

                    for (const auto index : indices) {
                        const auto& element = alice_account_.BalanceElement(
                            Subchain::External, index);
                        const auto key = element.Key();

                        OT_ASSERT(key);

                        output.emplace_back(
                            amount_,
                            miner_.Factory().BitcoinScriptP2PK(
                                test_chain_, *key),
                            keys);
                    }

                    return output;
                }(),
                coinbase_fun_);

            OT_ASSERT(output);

            transactions_.emplace_back(output->ID());

            return output;
        })
        , get_p2pk_bytes_(
              [&](const auto& script, auto index) { return script.Pubkey(); })
        , get_p2pk_patterns_([&](auto index) { return Pattern::PayToPubkey; })
        , listener_alice_([&]() -> ScanListener& {
            if (!listener_alice_p_) {
                listener_alice_p_ = std::make_unique<ScanListener>(client_1_);
            }

            OT_ASSERT(listener_alice_p_);

            return *listener_alice_p_;
        }())
        , listener_bob_([&]() -> ScanListener& {
            if (!listener_bob_p_) {
                listener_bob_p_ = std::make_unique<ScanListener>(client_2_);
            }

            OT_ASSERT(listener_bob_p_);

            return *listener_bob_p_;
        }())
    {
    }
};

ot::Nym_p Regtest_stress::alice_p_{};
ot::Nym_p Regtest_stress::bob_p_{};
Regtest_stress::Transactions Regtest_stress::transactions_{};
std::unique_ptr<ScanListener> Regtest_stress::listener_alice_p_{};
std::unique_ptr<ScanListener> Regtest_stress::listener_bob_p_{};

namespace
{
TEST_F(Regtest_stress, init_opentxs) {}

TEST_F(Regtest_stress, start_chains) { EXPECT_TRUE(Start()); }

TEST_F(Regtest_stress, connect_peers) { EXPECT_TRUE(Connect()); }

TEST_F(Regtest_stress, mine_initial_balance)
{
    auto future1 =
        listener_alice_.get_future(alice_account_, Subchain::External, 1);
    auto future2 =
        listener_alice_.get_future(alice_account_, Subchain::Internal, 1);

    std::cout << "Block 1\n";
    namespace c = std::chrono;
    EXPECT_TRUE(Mine(0, 1, mine_to_alice_));
    EXPECT_TRUE(listener_alice_.wait(future1));
    EXPECT_TRUE(listener_alice_.wait(future2));
}

TEST_F(Regtest_stress, alice_after_receive_wallet)
{
    const auto& network = client_1_.Blockchain().GetChain(test_chain_);
    const auto& wallet = network.Wallet();
    const auto& nym = alice_.ID();
    const auto& account = alice_account_.ID();
    const auto blankNym = client_1_.Factory().NymID();
    const auto blankAccount = client_1_.Factory().Identifier();
    using Balance = ot::blockchain::Balance;
    const auto outputs = tx_per_block_ * 2u;
    const auto amount = outputs * amount_;
    const auto balance = Balance{amount, amount};
    const auto noBalance = Balance{0, 0};
    // const auto outpointsConfirmedNew = [&] {
    //    auto out = std::vector<Outpoint>{};
    //    const auto& txid = transactions_.at(0);
    //
    //    for (auto i{0u}; i < outputs; ++i) {
    //        out.emplace_back(txid->Bytes(), i);
    //    }
    //
    //    return out;
    //}();
    // const auto keysConfirmedNew = [&] {
    //    auto out = std::vector<ot::OTData>{};
    //
    //    for (auto i{0u}; i < outputs; ++i) {
    //        const auto& element =
    //            alice_account_.BalanceElement(Subchain::External, i);
    //        const auto k = element.Key();
    //
    //        OT_ASSERT(k);
    //
    //        out.emplace_back(client_1_.Factory().Data(k->PublicKey()));
    //    }
    //
    //    return out;
    //}();
    // const auto testConfirmedNew = [&](const auto& utxos) -> bool {
    //    return TestUTXOs(
    //        outpointsConfirmedNew,
    //        keysConfirmedNew,
    //        utxos,
    //        get_p2pk_bytes_,
    //        get_p2pk_patterns_,
    //        [](const auto index) -> std::int64_t {
    //            const auto amount = ot::blockchain::Amount{amount_};
    //
    //            return amount;
    //        });
    //};

    // ASSERT_EQ(outpointsConfirmedNew.size(), outputs);
    // ASSERT_EQ(keysConfirmedNew.size(), outputs);

    EXPECT_EQ(wallet.GetBalance(), balance);
    EXPECT_EQ(network.GetBalance(), balance);
    EXPECT_EQ(wallet.GetBalance(nym), balance);
    EXPECT_EQ(network.GetBalance(nym), balance);
    EXPECT_EQ(wallet.GetBalance(nym, account), balance);
    EXPECT_EQ(wallet.GetBalance(blankNym), noBalance);
    EXPECT_EQ(network.GetBalance(blankNym), noBalance);
    EXPECT_EQ(wallet.GetBalance(blankNym, blankAccount), noBalance);
    EXPECT_EQ(wallet.GetBalance(nym, blankAccount), noBalance);
    EXPECT_EQ(wallet.GetBalance(blankNym, account), noBalance);

    using TxoState = ot::blockchain::client::Wallet::TxoState;
    auto type = TxoState::All;

    EXPECT_EQ(wallet.GetOutputs(type).size(), outputs);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), outputs);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), outputs);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(type)));
    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(nym, type)));
    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(nym, account, type)));

    type = TxoState::UnconfirmedNew;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    type = TxoState::UnconfirmedSpend;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    type = TxoState::ConfirmedNew;

    EXPECT_EQ(wallet.GetOutputs(type).size(), outputs);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), outputs);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), outputs);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(type)));
    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(nym, type)));
    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(nym, account, type)));

    type = TxoState::ConfirmedSpend;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    type = TxoState::OrphanedNew;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    type = TxoState::OrphanedSpend;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);
}

TEST_F(Regtest_stress, generate_transactions)
{
    namespace c = std::chrono;
    const auto& alice = client_1_.Blockchain().GetChain(test_chain_);
    const auto previous{1u};
    const auto stop = previous + blocks_;
    auto future1 =
        listener_bob_.get_future(bob_account_, Subchain::External, stop);
    auto transactions =
        std::vector<ot::OTData>{tx_per_block_, client_1_.Factory().Data()};
    using Future = ot::blockchain::Network::PendingOutgoing;
    auto futures = std::array<Future, tx_per_block_>{};

    ASSERT_EQ(transactions.size(), tx_per_block_);

    for (auto b{0u}; b < blocks_; ++b) {
        std::cout << "Block " << std::to_string(previous + b + 1) << '\n';
        const auto start = ot::Clock::now();
        const auto destinations = GetAddresses();
        const auto addresses = ot::Clock::now();
        std::cout
            << std::to_string(
                   c::duration_cast<c::seconds>(addresses - start).count())
            << " sec to calculate receiving addresses\n";

        for (auto t{0u}; t < tx_per_block_; ++t) {
            futures.at(t) =
                alice.SendToAddress(alice_.ID(), destinations.at(t), amount_);
        }

        const auto init = ot::Clock::now();
        std::cout << std::to_string(
                         c::duration_cast<c::seconds>(init - addresses).count())
                  << " sec to queue outgoing transactions\n";

        {
            auto f{-1};

            for (auto& future : futures) {
                using State = std::future_status;
                constexpr auto limit = std::chrono::minutes{10};

                while (State::ready != future.wait_for(limit)) { ; }

                try {
                    auto txid = future.get();

                    OT_ASSERT(false == txid->empty());

                    transactions.at(++f)->swap(std::move(txid.get()));
                } catch (...) {

                    OT_FAIL;
                }
            }
        }

        const auto sigs = ot::Clock::now();
        std::cout << std::to_string(
                         c::duration_cast<c::seconds>(sigs - init).count())
                  << " sec to sign and broadcast transactions\n";
        const auto extra = [&] {
            auto output = std::vector<Transaction>{};

            for (const auto& txid : transactions) {
                const auto& pTX = output.emplace_back(
                    client_1_.Blockchain().LoadTransactionBitcoin(txid));

                OT_ASSERT(pTX);
            }

            return output;
        }();

        const auto target = previous + b + 1;
        auto future3 = listener_alice_.get_future(
            alice_account_, Subchain::External, target);
        auto future4 = listener_alice_.get_future(
            alice_account_, Subchain::Internal, target);

        EXPECT_TRUE(Mine(previous + b, 1, mine_to_alice_, extra));

        const auto mined = ot::Clock::now();
        std::cout << std::to_string(
                         c::duration_cast<c::seconds>(mined - sigs).count())
                  << " sec to mine block\n";

        EXPECT_TRUE(listener_alice_.wait(future3));
        EXPECT_TRUE(listener_alice_.wait(future4));

        const auto scanned = ot::Clock::now();
        std::cout << std::to_string(
                         c::duration_cast<c::seconds>(scanned - mined).count())
                  << " sec to scan accounts\n";
    }

    EXPECT_TRUE(listener_bob_.wait(future1));
}

TEST_F(Regtest_stress, bob_after_receive)
{
    account_activity_.expected_ += transaction_count_ + 1u;
    const auto& widget = client_2_.UI().AccountActivity(
        bob_.ID(),
        expected_account_,
        make_cb(account_activity_, u8"account_activity_"));
    constexpr auto expectedTotal = amount_ * transaction_count_;

    EXPECT_TRUE(wait_for_counter(account_activity_));
    EXPECT_EQ(widget.Balance(), expectedTotal);
    EXPECT_EQ(widget.BalancePolarity(), 1);

    auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_EQ(row->Amount(), amount_);

    while (false == row->Last()) {
        row = widget.Next();

        EXPECT_EQ(row->Amount(), amount_);
    }
}

TEST_F(Regtest_stress, shutdown) { Shutdown(); }
}  // namespace
