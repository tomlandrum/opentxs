// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>

#include "Helpers.hpp"
#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "internal/api/client/Client.hpp"
#include "internal/blockchain/block/Block.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/blockchain/NumericHash.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/blockchain/block/Header.hpp"
#include "opentxs/blockchain/client/HeaderOracle.hpp"
#include "opentxs/core/Data.hpp"

namespace b = ot::blockchain;
namespace bb = b::block;
namespace bc = b::client;

namespace
{
class Test_BlockHeader : public ::testing::Test
{
public:
    const ot::api::client::internal::Manager& api_;

    Test_BlockHeader()
        : api_(dynamic_cast<const ot::api::client::internal::Manager&>(
              ot::Context().StartClient(OTTestEnvironment::test_args_, 0)))
    {
    }
};

TEST_F(Test_BlockHeader, init_opentxs) {}

TEST_F(Test_BlockHeader, btc_genesis_block_hash_oracle)
{
    const auto expectedHash =
        ot::Data::Factory(btc_genesis_hash_, ot::Data::Mode::Hex);
    const auto& genesisHash =
        bc::HeaderOracle::GenesisBlockHash(b::Type::Bitcoin);

    EXPECT_EQ(expectedHash.get(), genesisHash);
}

TEST_F(Test_BlockHeader, ltc_genesis_block_hash_oracle)
{
    const auto expectedHash =
        ot::Data::Factory(ltc_genesis_hash_, ot::Data::Mode::Hex);
    const auto& genesisHash =
        bc::HeaderOracle::GenesisBlockHash(b::Type::Litecoin);

    EXPECT_EQ(expectedHash.get(), genesisHash);
}

TEST_F(Test_BlockHeader, btc_genesis_block_header)
{
    const auto blankHash =
        ot::Data::Factory(std::string(blank_hash_), ot::Data::Mode::Hex);
    const auto expectedHash =
        ot::Data::Factory(btc_genesis_hash_, ot::Data::Mode::Hex);
    const std::string numericHash{btc_genesis_hash_numeric_};
    std::unique_ptr<const bb::Header> pHeader{
        ot::factory::GenesisBlockHeader(api_, b::Type::Bitcoin)};

    ASSERT_TRUE(pHeader);

    const auto& header = *pHeader;

    EXPECT_EQ(header.EffectiveState(), bb::Header::Status::Normal);
    EXPECT_EQ(expectedHash.get(), header.Hash());
    EXPECT_EQ(header.Height(), 0);
    EXPECT_EQ(header.InheritedState(), bb::Header::Status::Normal);
    EXPECT_FALSE(header.IsBlacklisted());
    EXPECT_FALSE(header.IsDisconnected());
    EXPECT_EQ(header.LocalState(), bb::Header::Status::Checkpoint);
    EXPECT_EQ(numericHash, header.NumericHash()->asHex());
    EXPECT_EQ(header.ParentHash(), blankHash.get());

    const auto [height, hash] = header.Position();

    EXPECT_EQ(header.Hash(), hash.get());
    EXPECT_EQ(header.Height(), height);
}

TEST_F(Test_BlockHeader, ltc_genesis_block_header)
{
    const auto blankHash =
        ot::Data::Factory(std::string(blank_hash_), ot::Data::Mode::Hex);
    const auto expectedHash =
        ot::Data::Factory(ltc_genesis_hash_, ot::Data::Mode::Hex);
    const std::string numericHash{ltc_genesis_hash_numeric_};
    std::unique_ptr<const bb::Header> pHeader{
        ot::factory::GenesisBlockHeader(api_, b::Type::Litecoin)};

    ASSERT_TRUE(pHeader);

    const auto& header = *pHeader;

    EXPECT_EQ(header.EffectiveState(), bb::Header::Status::Normal);
    EXPECT_EQ(expectedHash.get(), header.Hash());
    EXPECT_EQ(header.Height(), 0);
    EXPECT_EQ(header.InheritedState(), bb::Header::Status::Normal);
    EXPECT_FALSE(header.IsBlacklisted());
    EXPECT_FALSE(header.IsDisconnected());
    EXPECT_EQ(header.LocalState(), bb::Header::Status::Checkpoint);
    EXPECT_EQ(numericHash, header.NumericHash()->asHex());
    EXPECT_EQ(header.ParentHash(), blankHash.get());

    const auto [height, hash] = header.Position();

    EXPECT_EQ(header.Hash(), hash.get());
    EXPECT_EQ(header.Height(), height);
}

TEST_F(Test_BlockHeader, serialize_deserialize)
{
    const auto expectedHash =
        ot::Data::Factory(btc_genesis_hash_, ot::Data::Mode::Hex);
    std::unique_ptr<const bb::Header> pHeader{
        ot::factory::GenesisBlockHeader(api_, b::Type::Bitcoin)};

    ASSERT_TRUE(pHeader);

    const auto& header = *pHeader;

    auto bytes = ot::Space{};
    EXPECT_TRUE(header.Serialize(ot::writer(bytes), false));
    auto restored = api_.Factory().BlockHeader(ot::reader(bytes));

    ASSERT_TRUE(restored);
    EXPECT_EQ(expectedHash.get(), restored->Hash());

    EXPECT_EQ(restored->Difficulty(), header.Difficulty());
    EXPECT_EQ(restored->EffectiveState(), header.EffectiveState());
    EXPECT_EQ(restored->Hash(), header.Hash());
    EXPECT_EQ(restored->Height(), header.Height());
    EXPECT_EQ(restored->IncrementalWork(), header.IncrementalWork());
    EXPECT_EQ(restored->InheritedState(), header.InheritedState());
    EXPECT_EQ(restored->IsBlacklisted(), header.IsBlacklisted());
    EXPECT_EQ(restored->IsDisconnected(), header.IsDisconnected());
    EXPECT_EQ(restored->LocalState(), header.LocalState());
    EXPECT_EQ(restored->NumericHash(), header.NumericHash());
    EXPECT_EQ(restored->ParentHash(), header.ParentHash());
    EXPECT_EQ(restored->ParentWork(), header.ParentWork());
    EXPECT_EQ(restored->Position(), header.Position());
    EXPECT_EQ(restored->Print(), header.Print());
    EXPECT_EQ(restored->Target(), header.Target());
    EXPECT_EQ(restored->Type(), header.Type());
    EXPECT_EQ(restored->Valid(), header.Valid());
    EXPECT_EQ(restored->Work(), header.Work());
}
}  // namespace
