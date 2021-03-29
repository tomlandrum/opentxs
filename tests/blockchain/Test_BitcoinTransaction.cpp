// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <cstring>
#include <iosfwd>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "internal/api/client/Client.hpp"
#include "internal/blockchain/bitcoin/Bitcoin.hpp"
#include "internal/blockchain/block/bitcoin/Bitcoin.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Hash.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/blockchain/block/Outpoint.hpp"
#include "opentxs/blockchain/block/bitcoin/Input.hpp"
#include "opentxs/blockchain/block/bitcoin/Inputs.hpp"
#include "opentxs/blockchain/block/bitcoin/Output.hpp"
#include "opentxs/blockchain/block/bitcoin/Outputs.hpp"
#include "opentxs/blockchain/block/bitcoin/Script.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/protobuf/BlockchainTransaction.pb.h"
#include "opentxs/protobuf/Enums.pb.h"

namespace
{
const auto txid_hex_ = std::string{
    "b9451ab8cb828275480da016e97368fdfbfbd9e27dd9bd5d3e6d56d8cd14f301"};
const auto transaction_hex_ = std::string{
    "01000000035a19f341c42071f9cec7df37c4853c95d6aecc95e3bf19e3181d30d99552b8c9"
    "000000008a473044022025bca5dc0fe42aca5f07c9b3fe1b3f72113ffbc3522f8d3ebb2457"
    "f5bdf8f9b2022030ff687c00a63e810b21e447d3a57b2749ebea553cab763eb9b99e1b9839"
    "653b014104469f7eb54b90d90106b1a5412b41a23516028e81ad35e0418a4460707ae39a4b"
    "f0101b632260fb08979aba0ceea576b5400c7cf30b539b055ec4c0b96ab00984ffffffff5b"
    "72d3f4b6b72b3511bddd9994f28a91cc03212f200f71b91df13e711d58c1da000000008c49"
    "3046022100fbef2589b7c52a3be0fd8dd3624445da9c8930f0e51f6a33d76dc0ca0304473d"
    "0221009ec433ca6a9f16184db46468ff39cafaa9643021e0c66a1de1e6f9a6120927900141"
    "04b27f4de096ac6431eec4b807a0d3db3e9f9be48faab692d5559624acb1faf4334dd440eb"
    "f32a81506b7c49d8cf40e4b3f5c6b6e99fcb6d3e8a298174bd2b348dffffffff292e947388"
    "51718433a3168e43cab1c6a811e9a0f35b06b6cec60fea9abe0f43010000008a4730440220"
    "582813f2c2d7cbb84521f81d6c2a1147e5296e90bee05f583b3df108fdac72010220232b43"
    "a2e596cef59f82c8bfff1a310d85e7beb3e607076ff8966d6d374dc12b014104a8514ca511"
    "37c6d8a4befa476a7521197b886fceafa9f5c2830bea6df62792a6dd46f2b26812b250f13f"
    "ad473e5cab6dcceaa2d53cf2c82e8e03d95a0e70836bffffffff0240420f00000000001976"
    "a914429e6bd3c9a9ca4be00a4b2b02fd4f5895c1405988ac4083e81c000000001976a914e5"
    "5756cb5395a4b39369d0f1f0a640c12fd867b288ac00000000"};
const auto mutated_transaction_hex_ = std::string{
    "01000000035a19f341c42071f9cec7df37c4853c95d6aecc95e3bf19e3181d30d99552b8c9"
    "000000008c4d47003044022025bca5dc0fe42aca5f07c9b3fe1b3f72113ffbc3522f8d3ebb"
    "2457f5bdf8f9b2022030ff687c00a63e810b21e447d3a57b2749ebea553cab763eb9b99e1b"
    "9839653b014104469f7eb54b90d90106b1a5412b41a23516028e81ad35e0418a4460707ae3"
    "9a4bf0101b632260fb08979aba0ceea576b5400c7cf30b539b055ec4c0b96ab00984ffffff"
    "ff5b72d3f4b6b72b3511bddd9994f28a91cc03212f200f71b91df13e711d58c1da00000000"
    "8c493046022100fbef2589b7c52a3be0fd8dd3624445da9c8930f0e51f6a33d76dc0ca0304"
    "473d0221009ec433ca6a9f16184db46468ff39cafaa9643021e0c66a1de1e6f9a612092790"
    "014104b27f4de096ac6431eec4b807a0d3db3e9f9be48faab692d5559624acb1faf4334dd4"
    "40ebf32a81506b7c49d8cf40e4b3f5c6b6e99fcb6d3e8a298174bd2b348dffffffff292e94"
    "738851718433a3168e43cab1c6a811e9a0f35b06b6cec60fea9abe0f43010000008a473044"
    "0220582813f2c2d7cbb84521f81d6c2a1147e5296e90bee05f583b3df108fdac7201022023"
    "2b43a2e596cef59f82c8bfff1a310d85e7beb3e607076ff8966d6d374dc12b014104a8514c"
    "a51137c6d8a4befa476a7521197b886fceafa9f5c2830bea6df62792a6dd46f2b26812b250"
    "f13fad473e5cab6dcceaa2d53cf2c82e8e03d95a0e70836bffffffff0240420f0000000000"
    "1976a914429e6bd3c9a9ca4be00a4b2b02fd4f5895c1405988ac4083e81c000000001976a9"
    "14e55756cb5395a4b39369d0f1f0a640c12fd867b288ac00000000"};
const auto outpoint_hex_1_ = std::string{
    "5a19f341c42071f9cec7df37c4853c95d6aecc95e3bf19e3181d30d99552b8c900000000"};
const auto outpoint_hex_2_ = std::string{
    "5b72d3f4b6b72b3511bddd9994f28a91cc03212f200f71b91df13e711d58c1da00000000"};
const auto outpoint_hex_3_ = std::string{
    "292e94738851718433a3168e43cab1c6a811e9a0f35b06b6cec60fea9abe0f4301000000"};
const auto in_hex_1_ = std::string{
    "473044022025bca5dc0fe42aca5f07c9b3fe1b3f72113ffbc3522f8d3ebb2457f5bdf8f9b2"
    "022030ff687c00a63e810b21e447d3a57b2749ebea553cab763eb9b99e1b9839653b014104"
    "469f7eb54b90d90106b1a5412b41a23516028e81ad35e0418a4460707ae39a4bf0101b6322"
    "60fb08979aba0ceea576b5400c7cf30b539b055ec4c0b96ab00984"};
const auto in_hex_2_ = std::string{
    "493046022100fbef2589b7c52a3be0fd8dd3624445da9c8930f0e51f6a33d76dc0ca030447"
    "3d0221009ec433ca6a9f16184db46468ff39cafaa9643021e0c66a1de1e6f9a61209279001"
    "4104b27f4de096ac6431eec4b807a0d3db3e9f9be48faab692d5559624acb1faf4334dd440"
    "ebf32a81506b7c49d8cf40e4b3f5c6b6e99fcb6d3e8a298174bd2b348d"};
const auto in_hex_3_ = std::string{
    "4730440220582813f2c2d7cbb84521f81d6c2a1147e5296e90bee05f583b3df108fdac7201"
    "0220232b43a2e596cef59f82c8bfff1a310d85e7beb3e607076ff8966d6d374dc12b014104"
    "a8514ca51137c6d8a4befa476a7521197b886fceafa9f5c2830bea6df62792a6dd46f2b268"
    "12b250f13fad473e5cab6dcceaa2d53cf2c82e8e03d95a0e70836b"};

struct Test_BitcoinTransaction : public ::testing::Test {
    const ot::api::client::internal::Manager& api_;
    const ot::OTData tx_id_;
    const ot::OTData tx_bytes_;
    const ot::OTData mutated_bytes_;
    const ot::OTData outpoint_1_;
    const ot::OTData outpoint_2_;
    const ot::OTData outpoint_3_;
    const ot::OTData in_script_1_;
    const ot::OTData in_script_2_;
    const ot::OTData in_script_3_;

    using Pattern = ot::blockchain::block::bitcoin::Script::Pattern;
    using Position = ot::blockchain::block::bitcoin::Script::Position;

    Test_BitcoinTransaction()
        : api_(dynamic_cast<const ot::api::client::internal::Manager&>(
              ot::Context().StartClient({}, 0)))
        , tx_id_(api_.Factory().Data(txid_hex_, ot::StringStyle::Hex))
        , tx_bytes_(api_.Factory().Data(transaction_hex_, ot::StringStyle::Hex))
        , mutated_bytes_(api_.Factory().Data(
              mutated_transaction_hex_,
              ot::StringStyle::Hex))
        , outpoint_1_(
              api_.Factory().Data(outpoint_hex_1_, ot::StringStyle::Hex))
        , outpoint_2_(
              api_.Factory().Data(outpoint_hex_2_, ot::StringStyle::Hex))
        , outpoint_3_(
              api_.Factory().Data(outpoint_hex_3_, ot::StringStyle::Hex))
        , in_script_1_(api_.Factory().Data(in_hex_1_, ot::StringStyle::Hex))
        , in_script_2_(api_.Factory().Data(in_hex_2_, ot::StringStyle::Hex))
        , in_script_3_(api_.Factory().Data(in_hex_3_, ot::StringStyle::Hex))
    {
    }
};

TEST_F(Test_BitcoinTransaction, serialization)
{
    const auto transaction = ot::factory::BitcoinTransaction(
        api_,
        api_.Blockchain(),
        ot::blockchain::Type::Bitcoin,
        std::numeric_limits<std::size_t>::max(),
        ot::Clock::now(),
        ot::blockchain::bitcoin::EncodedTransaction::Deserialize(
            api_, ot::blockchain::Type::Bitcoin, tx_bytes_->Bytes()));

    ASSERT_TRUE(transaction);
    EXPECT_EQ(tx_id_.get(), transaction->ID());
    EXPECT_EQ(transaction->Locktime(), 0);
    EXPECT_EQ(transaction->Version(), 1);

    {
        const auto& inputs = transaction->Inputs();

        ASSERT_EQ(3, inputs.size());

        {
            const auto& input1 = inputs.at(0);

            ASSERT_EQ(sizeof(input1.PreviousOutput()), outpoint_1_->size());
            EXPECT_EQ(
                std::memcmp(
                    &input1.PreviousOutput(),
                    outpoint_1_->data(),
                    outpoint_1_->size()),
                0);

            const auto& script1 = input1.Script();

            EXPECT_EQ(Pattern::Input, script1.Type());
            EXPECT_EQ(Position::Input, script1.Role());

            auto bytes1 = ot::Space{};

            EXPECT_TRUE(script1.Serialize(ot::writer(bytes1)));
            ASSERT_EQ(bytes1.size(), in_script_1_->size());
            EXPECT_EQ(
                std::memcmp(
                    bytes1.data(), in_script_1_->data(), in_script_1_->size()),
                0);
            EXPECT_EQ(input1.Sequence(), 4294967295u);
        }

        {
            const auto& input2 = inputs.at(1);

            ASSERT_EQ(sizeof(input2.PreviousOutput()), outpoint_2_->size());
            EXPECT_EQ(
                std::memcmp(
                    &input2.PreviousOutput(),
                    outpoint_2_->data(),
                    outpoint_2_->size()),
                0);

            const auto& script2 = input2.Script();

            EXPECT_EQ(Pattern::Input, script2.Type());
            EXPECT_EQ(Position::Input, script2.Role());

            auto bytes2 = ot::Space{};

            EXPECT_TRUE(script2.Serialize(ot::writer(bytes2)));
            ASSERT_EQ(bytes2.size(), in_script_2_->size());
            EXPECT_EQ(
                std::memcmp(
                    bytes2.data(), in_script_2_->data(), in_script_2_->size()),
                0);
            EXPECT_EQ(4294967295u, input2.Sequence());
        }

        {
            const auto& input3 = inputs.at(2);

            ASSERT_EQ(sizeof(input3.PreviousOutput()), outpoint_3_->size());
            EXPECT_EQ(
                std::memcmp(
                    &input3.PreviousOutput(),
                    outpoint_3_->data(),
                    outpoint_3_->size()),
                0);

            const auto& script3 = input3.Script();

            EXPECT_EQ(Pattern::Input, script3.Type());
            EXPECT_EQ(Position::Input, script3.Role());

            auto bytes3 = ot::Space{};

            EXPECT_TRUE(script3.Serialize(ot::writer(bytes3)));
            ASSERT_EQ(bytes3.size(), in_script_3_->size());
            EXPECT_EQ(
                std::memcmp(
                    bytes3.data(), in_script_3_->data(), in_script_3_->size()),
                0);
            EXPECT_EQ(4294967295u, input3.Sequence());
        }
    }

    {
        const auto& outputs = transaction->Outputs();

        ASSERT_EQ(2, outputs.size());

        {
            const auto& output1 = outputs.at(0);

            EXPECT_EQ(1000000, output1.Value());

            const auto& script4 = output1.Script();

            EXPECT_EQ(Pattern::PayToPubkeyHash, script4.Type());
            EXPECT_EQ(Position::Output, script4.Role());
            EXPECT_TRUE(script4.PubkeyHash().has_value());
        }

        {
            const auto& output2 = outputs.at(1);

            EXPECT_EQ(485000000, output2.Value());

            const auto& script5 = output2.Script();

            EXPECT_EQ(Pattern::PayToPubkeyHash, script5.Type());
            EXPECT_EQ(Position::Output, script5.Role());
            EXPECT_TRUE(script5.PubkeyHash().has_value());
        }
    }

    auto raw = api_.Factory().Data();

    ASSERT_TRUE(transaction->Serialize(raw->WriteInto()));
    EXPECT_EQ(tx_bytes_->size(), raw->size());
    EXPECT_EQ(tx_bytes_.get(), raw.get());

    const auto serialized = transaction->Serialize(api_.Blockchain());

    ASSERT_TRUE(serialized.has_value());

    auto transaction2 = ot::factory::BitcoinTransaction(
        api_, api_.Blockchain(), serialized.value());

    ASSERT_TRUE(transaction2);
    EXPECT_EQ(transaction2->Locktime(), 0);
    EXPECT_EQ(transaction2->Version(), 1);

    {
        const auto& inputs = transaction2->Inputs();

        ASSERT_EQ(3, inputs.size());

        {
            const auto& input1 = inputs.at(0);

            ASSERT_EQ(sizeof(input1.PreviousOutput()), outpoint_1_->size());
            EXPECT_EQ(
                std::memcmp(
                    &input1.PreviousOutput(),
                    outpoint_1_->data(),
                    outpoint_1_->size()),
                0);

            const auto& script1 = input1.Script();

            EXPECT_EQ(Pattern::Input, script1.Type());
            EXPECT_EQ(Position::Input, script1.Role());

            auto bytes1 = ot::Space{};

            EXPECT_TRUE(script1.Serialize(ot::writer(bytes1)));
            ASSERT_EQ(bytes1.size(), in_script_1_->size());
            EXPECT_EQ(
                std::memcmp(
                    bytes1.data(), in_script_1_->data(), in_script_1_->size()),
                0);
            EXPECT_EQ(4294967295u, input1.Sequence());
        }

        {
            const auto& input2 = inputs.at(1);

            ASSERT_EQ(sizeof(input2.PreviousOutput()), outpoint_2_->size());
            EXPECT_EQ(
                std::memcmp(
                    &input2.PreviousOutput(),
                    outpoint_2_->data(),
                    outpoint_2_->size()),
                0);

            const auto& script2 = input2.Script();

            EXPECT_EQ(Pattern::Input, script2.Type());
            EXPECT_EQ(Position::Input, script2.Role());

            auto bytes2 = ot::Space{};

            EXPECT_TRUE(script2.Serialize(ot::writer(bytes2)));
            ASSERT_EQ(bytes2.size(), in_script_2_->size());
            EXPECT_EQ(
                std::memcmp(
                    bytes2.data(), in_script_2_->data(), in_script_2_->size()),
                0);
            EXPECT_EQ(4294967295u, input2.Sequence());
        }

        {
            const auto& input3 = inputs.at(2);

            ASSERT_EQ(sizeof(input3.PreviousOutput()), outpoint_3_->size());
            EXPECT_EQ(
                std::memcmp(
                    &input3.PreviousOutput(),
                    outpoint_3_->data(),
                    outpoint_3_->size()),
                0);

            const auto& script3 = input3.Script();

            EXPECT_EQ(Pattern::Input, script3.Type());
            EXPECT_EQ(Position::Input, script3.Role());

            auto bytes3 = ot::Space{};

            EXPECT_TRUE(script3.Serialize(ot::writer(bytes3)));
            ASSERT_EQ(bytes3.size(), in_script_3_->size());
            EXPECT_EQ(
                std::memcmp(
                    bytes3.data(), in_script_3_->data(), in_script_3_->size()),
                0);
            EXPECT_EQ(4294967295u, input3.Sequence());
        }
    }

    {
        const auto& outputs = transaction2->Outputs();

        ASSERT_EQ(2, outputs.size());

        {
            const auto& output1 = outputs.at(0);

            EXPECT_EQ(1000000, output1.Value());

            const auto& script4 = output1.Script();

            EXPECT_EQ(Pattern::PayToPubkeyHash, script4.Type());
            EXPECT_EQ(Position::Output, script4.Role());
            EXPECT_TRUE(script4.PubkeyHash().has_value());
        }

        {
            const auto& output2 = outputs.at(1);

            EXPECT_EQ(485000000, output2.Value());

            const auto& script5 = output2.Script();

            EXPECT_EQ(Pattern::PayToPubkeyHash, script5.Type());
            EXPECT_EQ(Position::Output, script5.Role());
            EXPECT_TRUE(script5.PubkeyHash().has_value());
        }
    }

    {
        const auto& bytes = serialized->serialized();

        ASSERT_EQ(bytes.size(), tx_bytes_->size());
        EXPECT_EQ(
            std::memcmp(bytes.data(), tx_bytes_->data(), tx_bytes_->size()), 0);
    }

    {
        auto bytes = ot::Space{};

        ASSERT_TRUE(transaction2->Serialize(ot::writer(bytes)));
        ASSERT_EQ(bytes.size(), tx_bytes_->size());
        EXPECT_EQ(
            std::memcmp(bytes.data(), tx_bytes_->data(), tx_bytes_->size()), 0);
    }
}

TEST_F(Test_BitcoinTransaction, normalized_id)
{
    const auto transaction1 = ot::factory::BitcoinTransaction(
        api_,
        api_.Blockchain(),
        ot::blockchain::Type::Bitcoin,
        std::numeric_limits<std::size_t>::max(),
        ot::Clock::now(),
        ot::blockchain::bitcoin::EncodedTransaction::Deserialize(
            api_, ot::blockchain::Type::Bitcoin, tx_bytes_->Bytes()));
    const auto transaction2 = ot::factory::BitcoinTransaction(
        api_,
        api_.Blockchain(),
        ot::blockchain::Type::Bitcoin,
        std::numeric_limits<std::size_t>::max(),
        ot::Clock::now(),
        ot::blockchain::bitcoin::EncodedTransaction::Deserialize(
            api_, ot::blockchain::Type::Bitcoin, mutated_bytes_->Bytes()));

    ASSERT_TRUE(transaction1);
    ASSERT_TRUE(transaction2);
    EXPECT_EQ(transaction1->IDNormalized(), transaction2->IDNormalized());

    auto id1 = api_.Factory().Data();
    auto id2 = api_.Factory().Data();

    ASSERT_TRUE(api_.Crypto().Hash().Digest(
        ot::crypto::HashType::Sha256D, tx_bytes_->Bytes(), id1->WriteInto()));
    ASSERT_TRUE(api_.Crypto().Hash().Digest(
        ot::crypto::HashType::Sha256D,
        mutated_bytes_->Bytes(),
        id2->WriteInto()));
    EXPECT_EQ(id1.get(), tx_id_.get());
    EXPECT_NE(id1.get(), id2.get());
}
}  // namespace
