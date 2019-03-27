// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "opentxs/opentxs.hpp"

#include "Internal.hpp"
#include "opentxs/crypto/library/Bitcoin.hpp"
#include "opentxs/crypto/library/Trezor.hpp"
#include "Factory.hpp"

#include <gtest/gtest.h>

namespace ot = ::opentxs;

namespace
{
class Test_Bitcoin_Providers : public ::testing::Test
{
public:
    const ot::api::client::Manager& client_;
    const ot::api::Crypto& crypto_;
#if OT_CRYPTO_USING_TREZOR
    const std::unique_ptr<ot::crypto::Trezor> trezor_{
        ot::Factory::Trezor(crypto_)};
#endif
#if OT_WITH_BLOCKCHAIN
    const std::unique_ptr<ot::crypto::Bitcoin> bitcoin_{
        ot::Factory::Bitcoin(crypto_)};
#endif
    const std::map<std::string, std::string> base_58_{
        {"", ""},
        {"00010966776006953D5567439E5E39F86A0D273BEE",
         "16UwLL9Risc3QfPqBUvKofHmBQ7wMtjvM"},
    };
    const std::map<std::string, std::string> ripemd160_{
        {"", "9c1185a5c5e9fc54612808977ee8f548b2258d31"},
        {"a", "0bdc9d2d256b3ee9daae347be6f4dc835a467ffe"},
        {"abc", "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc"},
        {"message digest", "5d0689ef49d2fae572b881b123a85ffa21595f36"},
        {"abcdefghijklmnopqrstuvwxyz",
         "f71c27109c692c1b56bbdceb5b9d2865b3708dbc"},
        {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
         "12a053384a9c0c88e405a06c27dcf49ada62eb2b"},
        {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
         "b0e20b6e3116640286ed3a87a5713079b21f5189"},
        {"123456789012345678901234567890123456789012345678901234567890123456789"
         "01234567890",
         "9b752e45573d4b39f4dbd3323cab82bf63326bfb"}};

    Test_Bitcoin_Providers()
        : client_(ot::OT::App().StartClient({}, 0))
        , crypto_(client_.Crypto())
    {
    }

    bool test_base58_encode(const ot::crypto::Bip32& library)
    {
        for (const auto& [key, value] : base_58_) {
            auto input = ot::Data::Factory();
            input->DecodeHex(key);
            const auto output = crypto_.Encode().IdentifierEncode(input);

            EXPECT_EQ(value, output);
        }

        return true;
    }

    bool test_base58_decode(const ot::crypto::Bip32& library)
    {
        for (const auto& [key, value] : base_58_) {
            auto expected = ot::Data::Factory();
            expected->DecodeHex(key);
            const auto decoded = crypto_.Encode().IdentifierDecode(value);
            const auto output =
                ot::Data::Factory(decoded.c_str(), decoded.size());

            EXPECT_EQ(expected.get(), output.get());
        }

        return true;
    }

    bool test_ripemd160(const ot::crypto::Bip32& library)
    {
        for (const auto& [preimage, hash] : ripemd160_) {
            auto input = ot::Data::Factory();
            auto output = ot::Data::Factory();

            EXPECT_TRUE(input->DecodeHex(hash));
            EXPECT_TRUE(crypto_.Hash().Digest(
                ot::proto::HASHTYPE_RIMEMD160, preimage, output));
            EXPECT_EQ(output.get(), input.get());
        }

        return true;
    }

    bool test_bip32_seed(const ot::crypto::Bip32& library)
    {
        // TODO test ot::crypto::Bip32::GetHDKey

        return true;
    }

    bool test_bip32_child_key(const ot::crypto::Bip32& library)
    {
        // TODO test ot::crypto::Bip32::GetChild

        return true;
    }

    bool test_bip32_seed_to_fingerprint(const ot::crypto::Bip32& library)
    {
        // TODO test ot::crypto::Bip32::SeedToFingerprint

        return true;
    }

    bool test_bip32_seed_to_key(const ot::crypto::Bip32& library)
    {
        // TODO test ot::crypto::Bip32::SeedToPrivateKey

        return true;
    }

    bool test_bip39_words(const ot::crypto::Bip32& library)
    {
        // TODO test ot::crypto::Bip39::SeedToWords

        return true;
    }

    bool test_bip39_seeds(const ot::crypto::Bip32& library)
    {
        // TODO test ot::crypto::Bip39::WordsToSeed

        return true;
    }
};

#if OT_CRYPTO_USING_TREZOR
TEST_F(Test_Bitcoin_Providers, Trezor)
{
    EXPECT_TRUE(test_base58_encode(*trezor_));
    EXPECT_TRUE(test_base58_decode(*trezor_));
    EXPECT_TRUE(test_ripemd160(*trezor_));
    EXPECT_TRUE(test_bip32_seed(*trezor_));
    EXPECT_TRUE(test_bip32_child_key(*trezor_));
    EXPECT_TRUE(test_bip32_seed_to_fingerprint(*trezor_));
    EXPECT_TRUE(test_bip32_seed_to_key(*trezor_));
    EXPECT_TRUE(test_bip39_words(*trezor_));
    EXPECT_TRUE(test_bip39_seeds(*trezor_));
}
#endif  // OT_CRYPTO_USING_TREZOR

#if OT_WITH_BLOCKCHAIN
TEST_F(Test_Bitcoin_Providers, Libbitcoin)
{
    EXPECT_TRUE(test_base58_encode(*bitcoin_));
    EXPECT_TRUE(test_base58_decode(*bitcoin_));
    EXPECT_TRUE(test_ripemd160(*bitcoin_));
    EXPECT_TRUE(test_bip32_seed(*bitcoin_));
    EXPECT_TRUE(test_bip32_child_key(*bitcoin_));
    EXPECT_TRUE(test_bip32_seed_to_fingerprint(*bitcoin_));
    EXPECT_TRUE(test_bip32_seed_to_key(*bitcoin_));
    EXPECT_TRUE(test_bip39_words(*bitcoin_));
    EXPECT_TRUE(test_bip39_seeds(*bitcoin_));
}
#endif  // OT_WITH_BLOCKCHAIN
}  // namespace
