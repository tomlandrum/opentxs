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
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/client/UI.hpp"
#include "opentxs/api/client/blockchain/BalanceNode.hpp"
#include "opentxs/api/client/blockchain/BalanceTree.hpp"
#include "opentxs/api/client/blockchain/HD.hpp"
#include "opentxs/api/client/blockchain/Subchain.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/blockchain/block/bitcoin/Outputs.hpp"
#include "opentxs/blockchain/block/bitcoin/Script.hpp"  // IWYU pragma: keep
#include "opentxs/blockchain/block/bitcoin/Transaction.hpp"
#include "opentxs/blockchain/client/BlockOracle.hpp"
#include "opentxs/blockchain/client/Wallet.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/crypto/key/EllipticCurve.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/ui/AccountActivity.hpp"
#include "opentxs/ui/AccountList.hpp"
#include "opentxs/ui/AccountListItem.hpp"
#include "opentxs/ui/BalanceItem.hpp"

Counter account_list_{};
Counter account_activity_{};

class Regtest_fixture_hd : public Regtest_fixture_normal
{
protected:
    using Subchain = ot::api::client::blockchain::Subchain;
    using UTXO = ot::blockchain::client::Wallet::UTXO;

    static ot::Nym_p alex_p_;
    static std::deque<ot::blockchain::block::pTxid> transactions_;
    static std::unique_ptr<ScanListener> listener_p_;

    const ot::identity::Nym& alex_;
    const ot::api::client::blockchain::HD& account_;
    const ot::Identifier& expected_account_;
    const ot::identifier::Server& expected_notary_;
    const ot::identifier::UnitDefinition& expected_unit_;
    const std::string expected_display_unit_;
    const std::string expected_account_name_;
    const std::string expected_notary_name_;
    const std::string memo_outgoing_;
    const ot::AccountType expected_account_type_;
    const ot::contact::ContactItemType expected_unit_type_;
    const Generator hd_generator_;
    const GetBytes get_p2pk_bytes_;
    const GetBytes get_p2pkh_bytes_;
    const GetPattern get_p2pk_patterns_;
    const GetPattern get_p2pkh_patterns_;
    ScanListener& listener_;

    auto Shutdown() noexcept -> void final
    {
        listener_p_.reset();
        transactions_.clear();
        alex_p_.reset();
        Regtest_fixture_normal::Shutdown();
    }

    Regtest_fixture_hd()
        : Regtest_fixture_normal(1)
        , alex_([&]() -> const ot::identity::Nym& {
            if (!alex_p_) {
                const auto reason =
                    client_1_.Factory().PasswordPrompt(__FUNCTION__);

                alex_p_ = client_1_.Wallet().Nym(reason, "Alex");

                OT_ASSERT(alex_p_)

                client_1_.Blockchain().NewHDSubaccount(
                    alex_p_->ID(),
                    ot::BlockchainAccountType::BIP44,
                    test_chain_,
                    reason);
            }

            OT_ASSERT(alex_p_)

            return *alex_p_;
        }())
        , account_(client_1_.Blockchain()
                       .Account(alex_.ID(), test_chain_)
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
        , hd_generator_([&](Height height) -> Transaction {
            using OutputBuilder = ot::api::Factory::OutputBuilder;

            auto output = miner_.Factory().BitcoinGenerationTransaction(
                test_chain_,
                height,
                [&] {
                    auto output = std::vector<OutputBuilder>{};
                    const auto reason =
                        client_1_.Factory().PasswordPrompt(__FUNCTION__);
                    const auto keys =
                        std::set<ot::api::client::blockchain::Key>{};
                    static const auto amount =
                        ot::blockchain::Amount{100000000};
                    using Index = ot::Bip32Index;

                    for (auto i = Index{0}; i < Index{100}; ++i) {
                        const auto index = account_.Reserve(
                            Subchain::External,
                            client_1_.Factory().PasswordPrompt(""));

                        const auto& element = account_.BalanceElement(
                            Subchain::External, index.value_or(0));
                        const auto key = element.Key();

                        OT_ASSERT(key);

                        switch (i) {
                            case 0: {
                                output.emplace_back(
                                    amount + i,
                                    miner_.Factory().BitcoinScriptP2PK(
                                        test_chain_, *key),
                                    keys);
                            } break;
                            default: {
                                output.emplace_back(
                                    amount + i,
                                    miner_.Factory().BitcoinScriptP2PKH(
                                        test_chain_, *key),
                                    keys);
                            }
                        }
                    }

                    return output;
                }(),
                coinbase_fun_);

            OT_ASSERT(output);

            transactions_.emplace_back(output->ID());

            return output;
        })
        , get_p2pk_bytes_([&](const auto& script, auto index) {
            if (0 == index) {

                return script.Pubkey();
            } else {

                return script.PubkeyHash();
            }
        })
        , get_p2pkh_bytes_([&](const auto& script, auto index) {
            return script.PubkeyHash();
        })
        , get_p2pk_patterns_([&](auto index) {
            if (0 == index) {

                return Pattern::PayToPubkey;
            } else {

                return Pattern::PayToPubkeyHash;
            }
        })
        , get_p2pkh_patterns_(
              [&](auto index) { return Pattern::PayToPubkeyHash; })
        , listener_([&]() -> ScanListener& {
            if (!listener_p_) {
                listener_p_ = std::make_unique<ScanListener>(client_1_);
            }

            OT_ASSERT(listener_p_);

            return *listener_p_;
        }())
    {
    }
};

ot::Nym_p Regtest_fixture_hd::alex_p_{};
std::deque<ot::blockchain::block::pTxid> Regtest_fixture_hd::transactions_{};
std::unique_ptr<ScanListener> Regtest_fixture_hd::listener_p_{};

namespace
{
TEST_F(Regtest_fixture_hd, init_opentxs) {}

TEST_F(Regtest_fixture_hd, start_chains) { EXPECT_TRUE(Start()); }

TEST_F(Regtest_fixture_hd, connect_peers) { EXPECT_TRUE(Connect()); }

TEST_F(Regtest_fixture_hd, init_account_list)
{
    account_list_.expected_ += 1;
    client_1_.UI().AccountList(
        alex_.ID(), make_cb(account_list_, u8"account_list_"));
    wait_for_counter(account_list_);
    const auto& widget = client_1_.UI().AccountList(alex_.ID());
    auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_EQ(row->AccountID(), expected_account_.str());
    EXPECT_EQ(row->Balance(), 0);
    EXPECT_EQ(row->ContractID(), expected_unit_.str());
    EXPECT_EQ(row->DisplayBalance(), u8"0 units");
    EXPECT_EQ(row->DisplayUnit(), expected_display_unit_);
    EXPECT_EQ(row->Name(), expected_account_name_);
    EXPECT_EQ(row->NotaryID(), expected_notary_.str());
    EXPECT_EQ(row->NotaryName(), expected_notary_name_);
    EXPECT_EQ(row->Type(), expected_account_type_);
    EXPECT_EQ(row->Unit(), expected_unit_type_);
    EXPECT_TRUE(row->Last());
}

TEST_F(Regtest_fixture_hd, init_account_activity)
{
    account_activity_.expected_ += 0;
    client_1_.UI().AccountActivity(
        alex_.ID(),
        client_1_.UI().BlockchainAccountID(test_chain_),
        make_cb(account_activity_, u8"account_activity_"));
    wait_for_counter(account_activity_);
    const auto& widget =
        client_1_.UI().AccountActivity(alex_.ID(), expected_account_);

    EXPECT_EQ(widget.AccountID(), expected_account_.str());
    EXPECT_EQ(widget.Balance(), 0);
    EXPECT_EQ(widget.BalancePolarity(), 0);
    EXPECT_EQ(widget.ContractID(), expected_unit_.str());
    EXPECT_FALSE(widget.DepositAddress().empty());
    EXPECT_FALSE(widget.DepositAddress(test_chain_).empty());
    EXPECT_TRUE(widget.DepositAddress(ot::blockchain::Type::Bitcoin).empty());

    const auto deposit = widget.DepositChains();

    ASSERT_EQ(deposit.size(), 1);
    EXPECT_EQ(deposit.at(0), test_chain_);
    EXPECT_EQ(widget.DisplayBalance(), u8"0 units");
    EXPECT_EQ(widget.DisplayUnit(), expected_display_unit_);
    EXPECT_EQ(widget.Name(), expected_account_name_);
    EXPECT_EQ(widget.NotaryID(), expected_notary_.str());
    EXPECT_EQ(widget.NotaryName(), expected_notary_name_);
    EXPECT_EQ(widget.SyncPercentage(), 0);

    constexpr auto progress = std::pair<int, int>{0, 0};

    EXPECT_EQ(widget.SyncProgress(), progress);
    EXPECT_EQ(widget.Type(), expected_account_type_);
    EXPECT_EQ(widget.Unit(), expected_unit_type_);
    EXPECT_TRUE(widget.ValidateAddress("mipcBbFg9gMiCh81Kj8tqqdgoZub1ZJRfn"));
    EXPECT_TRUE(widget.ValidateAddress("2MzQwSSnBHWHqSAqtTVQ6v47XtaisrJa1Vc"));
    EXPECT_TRUE(widget.ValidateAddress("2N2PJEucf6QY2kNFuJ4chQEBoyZWszRQE16"));
    EXPECT_FALSE(widget.ValidateAddress("17VZNX1SN5NtKa8UQFxwQbFeFc3iqRYhem"));
    EXPECT_FALSE(widget.ValidateAddress("3EktnHQD7RiAE6uzMj2ZifT9YgRrkSgzQX"));
    EXPECT_FALSE(widget.ValidateAddress("LM2WMpR1Rp6j3Sa59cMXMs1SPzj9eXpGc1"));
    EXPECT_FALSE(widget.ValidateAddress("MTf4tP1TCNBn8dNkyxeBVoPrFCcVzxJvvh"));
    EXPECT_FALSE(widget.ValidateAddress("QVk4MvUu7Wb7tZ1wvAeiUvdF7wxhvpyLLK"));
    EXPECT_FALSE(widget.ValidateAddress("pS8EA1pKEVBvv3kGsSGH37R8YViBmuRCPn"));

    auto row = widget.First();

    EXPECT_FALSE(row->Valid());
}

TEST_F(Regtest_fixture_hd, mine)
{
    auto future1 = listener_.get_future(account_, Subchain::External, 1);
    auto future2 = listener_.get_future(account_, Subchain::Internal, 1);
    constexpr auto count{1};
    account_list_.expected_ += count;
    account_activity_.expected_ += (2 * count);

    EXPECT_TRUE(Mine(0, count, hd_generator_));
    EXPECT_TRUE(listener_.wait(future1));
    EXPECT_TRUE(listener_.wait(future2));
}

TEST_F(Regtest_fixture_hd, first_block)
{
    const auto& blockchain = client_1_.Blockchain().GetChain(test_chain_);
    const auto blockHash = blockchain.HeaderOracle().BestHash(1);

    ASSERT_FALSE(blockHash->empty());

    const auto pBlock = blockchain.BlockOracle().LoadBitcoin(blockHash).get();

    ASSERT_TRUE(pBlock);

    const auto& block = *pBlock;

    ASSERT_EQ(block.size(), 1);

    const auto pTx = block.at(0);

    ASSERT_TRUE(pTx);

    const auto& tx = *pTx;

    EXPECT_EQ(tx.ID(), transactions_.at(0));
    EXPECT_EQ(tx.BlockPosition(), 0);
    EXPECT_EQ(tx.Outputs().size(), 100);
}

TEST_F(Regtest_fixture_hd, wallet_after_receive)
{
    const auto& network = client_1_.Blockchain().GetChain(test_chain_);
    const auto& wallet = network.Wallet();
    const auto& nym = alex_.ID();
    const auto& account = account_.ID();
    const auto blankNym = client_1_.Factory().NymID();
    const auto blankAccount = client_1_.Factory().Identifier();
    using Balance = ot::blockchain::Balance;
    const auto balance = Balance{10000004950, 10000004950};
    const auto noBalance = Balance{0, 0};

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

    EXPECT_EQ(wallet.GetOutputs(type).size(), 100u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 100u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 100u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

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

    EXPECT_EQ(wallet.GetOutputs(type).size(), 100u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 100u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 100u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

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

TEST_F(Regtest_fixture_hd, account_list_after_receive)
{
    wait_for_counter(account_list_);
    const auto& widget = client_1_.UI().AccountList(alex_.ID());
    auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_EQ(row->AccountID(), expected_account_.str());
    EXPECT_EQ(row->Balance(), 10000004950);
    EXPECT_EQ(row->ContractID(), expected_unit_.str());
    EXPECT_EQ(row->DisplayBalance(), u8"100.000 049 5 units");
    EXPECT_EQ(row->DisplayUnit(), expected_display_unit_);
    EXPECT_EQ(row->Name(), expected_account_name_);
    EXPECT_EQ(row->NotaryID(), expected_notary_.str());
    EXPECT_EQ(row->NotaryName(), expected_notary_name_);
    EXPECT_EQ(row->Type(), expected_account_type_);
    EXPECT_EQ(row->Unit(), expected_unit_type_);
    EXPECT_TRUE(row->Last());
}

TEST_F(Regtest_fixture_hd, account_activity_after_receive)
{
    wait_for_counter(account_activity_);
    const auto& widget =
        client_1_.UI().AccountActivity(alex_.ID(), expected_account_);

    EXPECT_EQ(widget.AccountID(), expected_account_.str());
    EXPECT_EQ(widget.Balance(), 10000004950);
    EXPECT_EQ(widget.BalancePolarity(), 1);
    EXPECT_EQ(widget.ContractID(), expected_unit_.str());
    EXPECT_FALSE(widget.DepositAddress().empty());
    EXPECT_FALSE(widget.DepositAddress(test_chain_).empty());
    EXPECT_TRUE(widget.DepositAddress(ot::blockchain::Type::Bitcoin).empty());

    const auto deposit = widget.DepositChains();

    ASSERT_EQ(deposit.size(), 1);
    EXPECT_EQ(deposit.at(0), test_chain_);
    EXPECT_EQ(widget.DisplayBalance(), u8"100.000 049 5 units");
    EXPECT_EQ(widget.DisplayUnit(), expected_display_unit_);
    EXPECT_EQ(widget.Name(), expected_account_name_);
    EXPECT_EQ(widget.NotaryID(), expected_notary_.str());
    EXPECT_EQ(widget.NotaryName(), expected_notary_name_);
    EXPECT_EQ(widget.SyncPercentage(), 100);

    constexpr auto progress = std::pair<int, int>{1, 1};

    EXPECT_EQ(widget.SyncProgress(), progress);
    EXPECT_EQ(widget.Type(), expected_account_type_);
    EXPECT_EQ(widget.Unit(), expected_unit_type_);

    auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_EQ(row->Amount(), 10000004950);
    EXPECT_EQ(row->Contacts().size(), 0);
    EXPECT_EQ(row->DisplayAmount(), u8"100.000 049 5 units");
    EXPECT_EQ(row->Memo(), "");
    EXPECT_EQ(row->Workflow(), "");
    EXPECT_EQ(row->Text(), "Incoming Unit Test Simulation transaction");
    EXPECT_EQ(row->Type(), ot::StorageBox::BLOCKCHAIN);
    EXPECT_EQ(row->UUID(), transactions_.at(0)->asHex());
    EXPECT_TRUE(row->Last());
}

TEST_F(Regtest_fixture_hd, spend)
{
    account_list_.expected_ += 1;
    account_activity_.expected_ += 2;
    const auto& network = client_1_.Blockchain().GetChain(test_chain_);
    const auto& widget =
        client_1_.UI().AccountActivity(alex_.ID(), expected_account_);
    constexpr auto sendAmount{u8"14 units"};
    constexpr auto address{"mipcBbFg9gMiCh81Kj8tqqdgoZub1ZJRfn"};

    ASSERT_FALSE(widget.ValidateAmount(sendAmount).empty());
    ASSERT_TRUE(widget.ValidateAddress(address));

    auto future =
        network.SendToAddress(alex_.ID(), address, 1400000000, memo_outgoing_);
    const auto& txid = transactions_.emplace_back(future.get());

    EXPECT_FALSE(txid->empty());
}

TEST_F(Regtest_fixture_hd, wallet_after_unconfirmed_spend)
{
    const auto& network = client_1_.Blockchain().GetChain(test_chain_);
    const auto& wallet = network.Wallet();
    const auto& nym = alex_.ID();
    const auto& account = account_.ID();
    const auto blankNym = client_1_.Factory().NymID();
    const auto blankAccount = client_1_.Factory().Identifier();
    using Balance = ot::blockchain::Balance;
    const auto balance = Balance{10000004950, 8600002652};
    const auto noBalance = Balance{0, 0};
    // using Index = ot::Bip32Index;
    // const auto outpointsUnconfirmedNew = [&] {
    //     auto out = std::vector<Outpoint>{};
    //     const auto& txid = transactions_.at(1);
    //     out.emplace_back(txid->Bytes(), 0);
    //
    //     return out;
    // }();
    // const auto outpointsUnconfirmedSpend = [&] {
    //     auto out = std::vector<Outpoint>{};
    //     const auto& txid = transactions_.at(0);
    //
    //     for (auto i = Index{0}; i < Index{15}; ++i) {
    //         out.emplace_back(txid->Bytes(), i);
    //     }
    //
    //     return out;
    // }();
    // const auto outpointsConfirmedNew = [&] {
    //     auto out = std::vector<Outpoint>{};
    //     const auto& txid = transactions_.at(0);
    //
    //     for (auto i = Index{15}; i < Index{100}; ++i) {
    //         out.emplace_back(txid->Bytes(), i);
    //     }
    //
    //     return out;
    // }();
    // const auto keysUnconfirmedNew = [&] {
    //     auto out = std::vector<ot::OTData>{};
    //     const auto& element = account_.BalanceElement(Subchain::Internal, 0);
    //     out.emplace_back(element.PubkeyHash());
    //
    //     return out;
    // }();
    // const auto keysUnconfirmedSpend = [&] {
    //     auto out = std::vector<ot::OTData>{};
    //
    //     for (auto i = Index{0}; i < Index{15}; ++i) {
    //         const auto& element =
    //             account_.BalanceElement(Subchain::External, i + 2u);
    //
    //         switch (i) {
    //             case 0: {
    //                 const auto k = element.Key();
    //
    //                 OT_ASSERT(k);
    //
    //                 out.emplace_back(client_1_.Factory().Data(k->PublicKey()));
    //             } break;
    //             default: {
    //                 out.emplace_back(element.PubkeyHash());
    //             }
    //         }
    //     }
    //
    //     return out;
    // }();
    // const auto keysConfirmedNew = [&] {
    //     auto out = std::vector<ot::OTData>{};
    //
    //     for (auto i = Index{15}; i < Index{100}; ++i) {
    //         const auto& element =
    //             account_.BalanceElement(Subchain::External, i + 2u);
    //         out.emplace_back(element.PubkeyHash());
    //     }
    //
    //     return out;
    // }();
    // const auto testUnconfirmedNew = [&](const auto& utxos) -> bool {
    //     return TestUTXOs(
    //         outpointsUnconfirmedNew,
    //         keysUnconfirmedNew,
    //         utxos,
    //         get_p2pkh_bytes_,
    //         get_p2pkh_patterns_,
    //         [](const auto index) -> std::int64_t {
    //             const auto amount = ot::blockchain::Amount{99997807};
    //
    //             return amount;
    //         });
    // };
    // const auto testUnconfirmedSpend = [&](const auto& utxos) -> bool {
    //     return TestUTXOs(
    //         outpointsUnconfirmedSpend,
    //         keysUnconfirmedSpend,
    //         utxos,
    //         get_p2pk_bytes_,
    //         get_p2pk_patterns_,
    //         [](const auto index) -> std::int64_t {
    //             const auto amount = ot::blockchain::Amount{100000000};
    //
    //             return amount + index;
    //         });
    // };
    // const auto testConfirmedNew = [&](const auto& utxos) -> bool {
    //     return TestUTXOs(
    //         outpointsConfirmedNew,
    //         keysConfirmedNew,
    //         utxos,
    //         get_p2pkh_bytes_,
    //         get_p2pkh_patterns_,
    //         [](const auto index) -> std::int64_t {
    //             const auto amount = ot::blockchain::Amount{100000000};
    //
    //             return amount + index + 15;
    //         });
    // };

    // ASSERT_EQ(outpointsUnconfirmedSpend.size(), 15u);
    // ASSERT_EQ(keysUnconfirmedSpend.size(), 15u);
    // ASSERT_EQ(outpointsUnconfirmedNew.size(), 1u);
    // ASSERT_EQ(keysUnconfirmedNew.size(), 1u);
    // ASSERT_EQ(outpointsConfirmedNew.size(), 85u);
    // ASSERT_EQ(keysConfirmedNew.size(), 85u);

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

    EXPECT_EQ(wallet.GetOutputs(type).size(), 101u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 101u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 101u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    type = TxoState::UnconfirmedNew;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    // EXPECT_TRUE(testUnconfirmedNew(wallet.GetOutputs(type)));
    // EXPECT_TRUE(testUnconfirmedNew(wallet.GetOutputs(nym, type)));
    // EXPECT_TRUE(testUnconfirmedNew(wallet.GetOutputs(nym, account, type)));

    type = TxoState::UnconfirmedSpend;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 15u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 15u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 15u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    // EXPECT_TRUE(testUnconfirmedSpend(wallet.GetOutputs(type)));
    // EXPECT_TRUE(testUnconfirmedSpend(wallet.GetOutputs(nym, type)));
    // EXPECT_TRUE(testUnconfirmedSpend(wallet.GetOutputs(nym, account, type)));

    type = TxoState::ConfirmedNew;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 85u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 85u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 85u);
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

TEST_F(Regtest_fixture_hd, account_list_after_spend)
{
    wait_for_counter(account_list_);
    const auto& widget = client_1_.UI().AccountList(alex_.ID());
    auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_EQ(row->AccountID(), expected_account_.str());
    EXPECT_EQ(row->Balance(), 8600002652);
    EXPECT_EQ(row->ContractID(), expected_unit_.str());
    EXPECT_EQ(row->DisplayBalance(), u8"86.000 026 52 units");
    EXPECT_EQ(row->DisplayUnit(), expected_display_unit_);
    EXPECT_EQ(row->Name(), expected_account_name_);
    EXPECT_EQ(row->NotaryID(), expected_notary_.str());
    EXPECT_EQ(row->NotaryName(), expected_notary_name_);
    EXPECT_EQ(row->Type(), expected_account_type_);
    EXPECT_EQ(row->Unit(), expected_unit_type_);
    EXPECT_TRUE(row->Last());
}

TEST_F(Regtest_fixture_hd, account_activity_after_unconfirmed_spend)
{
    wait_for_counter(account_activity_);
    const auto& widget =
        client_1_.UI().AccountActivity(alex_.ID(), expected_account_);

    EXPECT_EQ(widget.AccountID(), expected_account_.str());
    EXPECT_EQ(widget.Balance(), 8600002652);
    EXPECT_EQ(widget.BalancePolarity(), 1);
    EXPECT_EQ(widget.ContractID(), expected_unit_.str());
    EXPECT_FALSE(widget.DepositAddress().empty());
    EXPECT_FALSE(widget.DepositAddress(test_chain_).empty());
    EXPECT_TRUE(widget.DepositAddress(ot::blockchain::Type::Bitcoin).empty());

    const auto deposit = widget.DepositChains();

    ASSERT_EQ(deposit.size(), 1);
    EXPECT_EQ(deposit.at(0), test_chain_);
    EXPECT_EQ(widget.DisplayBalance(), u8"86.000 026 52 units");
    EXPECT_EQ(widget.DisplayUnit(), expected_display_unit_);
    EXPECT_EQ(widget.Name(), expected_account_name_);
    EXPECT_EQ(widget.NotaryID(), expected_notary_.str());
    EXPECT_EQ(widget.NotaryName(), expected_notary_name_);
    EXPECT_EQ(widget.SyncPercentage(), 100);

    constexpr auto progress = std::pair<int, int>{1, 1};

    EXPECT_EQ(widget.SyncProgress(), progress);
    EXPECT_EQ(widget.Type(), expected_account_type_);
    EXPECT_EQ(widget.Unit(), expected_unit_type_);

    auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_EQ(row->Amount(), -1400002298);
    EXPECT_EQ(row->Contacts().size(), 0);
    EXPECT_EQ(row->DisplayAmount(), u8"-14.000 022 98 units");
    EXPECT_EQ(row->Memo(), "");
    EXPECT_EQ(row->Workflow(), "");
    EXPECT_EQ(row->Text(), "Outgoing Unit Test Simulation transaction");
    EXPECT_EQ(row->Type(), ot::StorageBox::BLOCKCHAIN);
    EXPECT_FALSE(row->UUID().empty());
    ASSERT_FALSE(row->Last());

    transactions_.emplace_back(
        client_1_.Factory().Data(row->UUID(), ot::StringStyle::Hex));
    row = widget.Next();

    EXPECT_EQ(row->Amount(), 10000004950);
    EXPECT_EQ(row->Contacts().size(), 0);
    EXPECT_EQ(row->DisplayAmount(), u8"100.000 049 5 units");
    EXPECT_EQ(row->Memo(), "");
    EXPECT_EQ(row->Workflow(), "");
    EXPECT_EQ(row->Text(), "Incoming Unit Test Simulation transaction");
    EXPECT_EQ(row->Type(), ot::StorageBox::BLOCKCHAIN);
    EXPECT_EQ(row->UUID(), transactions_.at(0)->asHex());
}

TEST_F(Regtest_fixture_hd, confirm)
{
    auto future1 = listener_.get_future(account_, Subchain::External, 2);
    auto future2 = listener_.get_future(account_, Subchain::Internal, 2);
    account_list_.expected_ += 0;
    account_activity_.expected_ += 1;
    const auto& txid = transactions_.at(1).get();
    const auto extra = [&] {
        auto output = std::vector<Transaction>{};
        const auto& pTX = output.emplace_back(
            client_1_.Blockchain().LoadTransactionBitcoin(txid));

        OT_ASSERT(pTX);

        return output;
    }();

    EXPECT_TRUE(Mine(1, 1, default_, extra));
    EXPECT_TRUE(listener_.wait(future1));
    EXPECT_TRUE(listener_.wait(future2));
}

TEST_F(Regtest_fixture_hd, wallet_after_confirmed_spend)
{
    const auto& network = client_1_.Blockchain().GetChain(test_chain_);
    const auto& wallet = network.Wallet();
    const auto& nym = alex_.ID();
    const auto& account = account_.ID();
    const auto blankNym = client_1_.Factory().NymID();
    const auto blankAccount = client_1_.Factory().Identifier();
    using Balance = ot::blockchain::Balance;
    const auto balance = Balance{8600002652, 8600002652};
    const auto noBalance = Balance{0, 0};
    // using Index = ot::Bip32Index;
    // const auto outpointsConfirmedNew = [&] {
    //     auto out = std::vector<Outpoint>{};
    //     {
    //         const auto& txid = transactions_.at(0);
    //
    //         for (auto i = Index{15}; i < Index{100}; ++i) {
    //             out.emplace_back(txid->Bytes(), i);
    //         }
    //     }
    //     {
    //         const auto& txid = transactions_.at(1);
    //         out.emplace_back(txid->Bytes(), 0);
    //     }
    //
    //     return out;
    // }();
    // const auto outpointsConfirmedSpend = [&] {
    //     const auto& txid = transactions_.at(0);
    //
    //     auto out = std::vector<Outpoint>{};
    //
    //     for (auto i = Index{0}; i < Index{15}; ++i) {
    //         out.emplace_back(txid->Bytes(), i);
    //     }
    //
    //     return out;
    // }();
    // const auto keysConfirmedNew = [&] {
    //     auto out = std::vector<ot::OTData>{};
    //
    //     for (auto i = Index{15}; i < Index{100}; ++i) {
    //         const auto& element =
    //             account_.BalanceElement(Subchain::External, i + 2u);
    //         out.emplace_back(element.PubkeyHash());
    //     }
    //
    //     const auto& element = account_.BalanceElement(Subchain::Internal, 0);
    //     out.emplace_back(element.PubkeyHash());
    //
    //     return out;
    // }();
    // const auto keysConfirmedSpend = [&] {
    //     auto out = std::vector<ot::OTData>{};
    //
    //     for (auto i = Index{0}; i < Index{15}; ++i) {
    //         const auto& element =
    //             account_.BalanceElement(Subchain::External, i + 2u);
    //
    //         switch (i) {
    //             case 0: {
    //                 const auto k = element.Key();
    //
    //                 OT_ASSERT(k);
    //
    //                 out.emplace_back(client_1_.Factory().Data(k->PublicKey()));
    //             } break;
    //             default: {
    //                 out.emplace_back(element.PubkeyHash());
    //             }
    //         }
    //     }
    //
    //     return out;
    // }();
    // const auto testConfirmedNew = [&](const auto& utxos) -> bool {
    //     return TestUTXOs(
    //         outpointsConfirmedNew,
    //         keysConfirmedNew,
    //         utxos,
    //         get_p2pkh_bytes_,
    //         get_p2pkh_patterns_,
    //         [](const auto index) -> std::int64_t {
    //             const auto amount = ot::blockchain::Amount{100000000};
    //
    //             if (index < 85) {
    //
    //                 return amount + index + 15;
    //             } else {
    //
    //                 return ot::blockchain::Amount{99997807};
    //             }
    //         });
    // };
    // const auto testConfirmedSpend = [&](const auto& utxos) -> bool {
    //     return TestUTXOs(
    //         outpointsConfirmedSpend,
    //         keysConfirmedSpend,
    //         utxos,
    //         get_p2pk_bytes_,
    //         get_p2pk_patterns_,
    //         [](const auto index) -> std::int64_t {
    //             const auto amount = ot::blockchain::Amount{100000000};
    //
    //             return amount + index;
    //         });
    // };
    //
    // ASSERT_EQ(outpointsConfirmedNew.size(), 86u);
    // ASSERT_EQ(keysConfirmedNew.size(), 86u);
    // ASSERT_EQ(outpointsConfirmedSpend.size(), 15u);
    // ASSERT_EQ(keysConfirmedSpend.size(), 15u);

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

    EXPECT_EQ(wallet.GetOutputs(type).size(), 101u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 101u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 101u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

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

    EXPECT_EQ(wallet.GetOutputs(type).size(), 86u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 86u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 86u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(type)));
    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(nym, type)));
    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(nym, account, type)));

    type = TxoState::ConfirmedSpend;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 15u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 15u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 15u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    // EXPECT_TRUE(testConfirmedSpend(wallet.GetOutputs(type)));
    // EXPECT_TRUE(testConfirmedSpend(wallet.GetOutputs(nym, type)));
    // EXPECT_TRUE(testConfirmedSpend(wallet.GetOutputs(nym, account, type)));

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

TEST_F(Regtest_fixture_hd, account_activity_after_confirmed_spend)
{
    wait_for_counter(account_activity_);
    const auto& widget =
        client_1_.UI().AccountActivity(alex_.ID(), expected_account_);

    EXPECT_EQ(widget.AccountID(), expected_account_.str());
    EXPECT_EQ(widget.Balance(), 8600002652);
    EXPECT_EQ(widget.BalancePolarity(), 1);
    EXPECT_EQ(widget.ContractID(), expected_unit_.str());
    EXPECT_FALSE(widget.DepositAddress().empty());
    EXPECT_FALSE(widget.DepositAddress(test_chain_).empty());
    EXPECT_TRUE(widget.DepositAddress(ot::blockchain::Type::Bitcoin).empty());

    const auto deposit = widget.DepositChains();

    ASSERT_EQ(deposit.size(), 1);
    EXPECT_EQ(deposit.at(0), test_chain_);
    EXPECT_EQ(widget.DisplayBalance(), u8"86.000 026 52 units");
    EXPECT_EQ(widget.DisplayUnit(), expected_display_unit_);
    EXPECT_EQ(widget.Name(), expected_account_name_);
    EXPECT_EQ(widget.NotaryID(), expected_notary_.str());
    EXPECT_EQ(widget.NotaryName(), expected_notary_name_);
    EXPECT_EQ(widget.SyncPercentage(), 100);

    constexpr auto progress = std::pair<int, int>{2, 2};

    EXPECT_EQ(widget.SyncProgress(), progress);
    EXPECT_EQ(widget.Type(), expected_account_type_);
    EXPECT_EQ(widget.Unit(), expected_unit_type_);

    auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_EQ(row->Amount(), -1400002298);
    EXPECT_EQ(row->Contacts().size(), 0);
    EXPECT_EQ(row->DisplayAmount(), u8"-14.000 022 98 units");
    EXPECT_EQ(row->Memo(), "");
    EXPECT_EQ(row->Workflow(), "");
    EXPECT_EQ(row->Text(), "Outgoing Unit Test Simulation transaction");
    EXPECT_EQ(row->Type(), ot::StorageBox::BLOCKCHAIN);
    EXPECT_FALSE(row->UUID().empty());
    ASSERT_FALSE(row->Last());

    transactions_.emplace_back(
        client_1_.Factory().Data(row->UUID(), ot::StringStyle::Hex));
    row = widget.Next();

    EXPECT_EQ(row->Amount(), 10000004950);
    EXPECT_EQ(row->Contacts().size(), 0);
    EXPECT_EQ(row->DisplayAmount(), u8"100.000 049 5 units");
    EXPECT_EQ(row->Memo(), "");
    EXPECT_EQ(row->Workflow(), "");
    EXPECT_EQ(row->Text(), "Incoming Unit Test Simulation transaction");
    EXPECT_EQ(row->Type(), ot::StorageBox::BLOCKCHAIN);
    EXPECT_EQ(row->UUID(), transactions_.at(0)->asHex());
}

TEST_F(Regtest_fixture_hd, account_list_after_confirmed_spend)
{
    wait_for_counter(account_list_);
    const auto& widget = client_1_.UI().AccountList(alex_.ID());
    auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_EQ(row->AccountID(), expected_account_.str());
    EXPECT_EQ(row->Balance(), 8600002652);
    EXPECT_EQ(row->ContractID(), expected_unit_.str());
    EXPECT_EQ(row->DisplayBalance(), u8"86.000 026 52 units");
    EXPECT_EQ(row->DisplayUnit(), expected_display_unit_);
    EXPECT_EQ(row->Name(), expected_account_name_);
    EXPECT_EQ(row->NotaryID(), expected_notary_.str());
    EXPECT_EQ(row->NotaryName(), expected_notary_name_);
    EXPECT_EQ(row->Type(), expected_account_type_);
    EXPECT_EQ(row->Unit(), expected_unit_type_);
    EXPECT_TRUE(row->Last());
}

TEST_F(Regtest_fixture_hd, reorg)
{
    auto future1 = listener_.get_future(account_, Subchain::External, 3);
    auto future2 = listener_.get_future(account_, Subchain::Internal, 3);

    EXPECT_TRUE(Mine(1, 2, default_));
    EXPECT_TRUE(listener_.wait(future1));
    EXPECT_TRUE(listener_.wait(future2));
}

TEST_F(Regtest_fixture_hd, shutdown) { Shutdown(); }
}  // namespace
