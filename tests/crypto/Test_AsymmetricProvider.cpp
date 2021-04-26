// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "internal/api/client/Client.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/HDSeed.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/client/OTAPI_Exec.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/crypto/Bip32Child.hpp"
#include "opentxs/crypto/Bip43Purpose.hpp"
#include "opentxs/crypto/SecretStyle.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"
#include "opentxs/crypto/key/HD.hpp"
#include "opentxs/crypto/library/AsymmetricProvider.hpp"
#include "opentxs/crypto/library/EcdsaProvider.hpp"
#include "util/HDIndex.hpp"  // IWYU pragma: keep

using namespace opentxs;

namespace
{
class Test_Signatures : public ::testing::Test
{
public:
    const ot::api::client::internal::Manager& client_;
#if OT_CRYPTO_WITH_BIP32
    const std::string fingerprint_;
#endif  // OT_CRYPTO_WITH_BIP32
    const crypto::HashType sha256_{crypto::HashType::Sha256};
    const crypto::HashType sha512_{crypto::HashType::Sha512};
    const crypto::HashType blake160_{crypto::HashType::Blake2b160};
    const crypto::HashType blake256_{crypto::HashType::Blake2b256};
    const crypto::HashType blake512_{crypto::HashType::Blake2b512};
    const crypto::HashType ripemd160_{crypto::HashType::Ripemd160};
    const std::string plaintext_string_1_{"Test string"};
    const std::string plaintext_string_2_{"Another string"};
    const OTData plaintext_1{
        Data::Factory(plaintext_string_1_.data(), plaintext_string_1_.size())};
    const OTData plaintext_2{
        Data::Factory(plaintext_string_2_.data(), plaintext_string_2_.size())};
#if OT_CRYPTO_SUPPORTED_KEY_ED25519
    OTAsymmetricKey ed_;
#if OT_CRYPTO_WITH_BIP32
    OTAsymmetricKey ed_hd_;
#endif  // OT_CRYPTO_WITH_BIP32
    OTAsymmetricKey ed_2_;
    const crypto::AsymmetricProvider& ed25519_;
#endif  // OT_CRYPTO_SUPPORTED_KEY_ED25519
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    OTAsymmetricKey secp_;
#if OT_CRYPTO_WITH_BIP32
    OTAsymmetricKey secp_hd_;
#endif  // OT_CRYPTO_WITH_BIP32
    OTAsymmetricKey secp_2_;
    const crypto::AsymmetricProvider& secp256k1_;
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
#if OT_CRYPTO_SUPPORTED_KEY_RSA
    OTAsymmetricKey rsa_;
    OTAsymmetricKey rsa_2_;
    const crypto::AsymmetricProvider& openssl_;
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA

    [[maybe_unused]] Test_Signatures()
        : client_(dynamic_cast<const ot::api::client::internal::Manager&>(
              ot::Context().StartClient({}, 0)))
#if OT_CRYPTO_WITH_BIP32
        , fingerprint_(client_.Exec().Wallet_ImportSeed(
              "response seminar brave tip suit recall often sound stick owner "
              "lottery motion",
              ""))
#endif  // OT_CRYPTO_WITH_BIP32
#if OT_CRYPTO_SUPPORTED_KEY_ED25519
        , ed_(get_key(client_, EcdsaCurve::ed25519))
#if OT_CRYPTO_WITH_BIP32
        , ed_hd_(get_hd_key(client_, fingerprint_, EcdsaCurve::ed25519))
        , ed_2_(get_hd_key(client_, fingerprint_, EcdsaCurve::ed25519, 1))
#else
        , ed_2_(get_key(client_, EcdsaCurve::ed25519))
#endif  // OT_CRYPTO_WITH_BIP32
        , ed25519_(client_.Crypto().ED25519())
#endif  // OT_CRYPTO_SUPPORTED_KEY_ED25519
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
        , secp_(get_key(client_, EcdsaCurve::secp256k1))
#if OT_CRYPTO_WITH_BIP32
        , secp_hd_(get_hd_key(client_, fingerprint_, EcdsaCurve::secp256k1))
        , secp_2_(get_hd_key(client_, fingerprint_, EcdsaCurve::secp256k1, 1))
#else
        , secp_2_(get_key(client_, EcdsaCurve::secp256k1))
#endif  // OT_CRYPTO_WITH_BIP32
        , secp256k1_(client_.Crypto().SECP256K1())
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
#if OT_CRYPTO_SUPPORTED_KEY_RSA
        , rsa_(get_key(client_, EcdsaCurve::invalid))
        , rsa_2_(get_key(client_, EcdsaCurve::invalid))
        , openssl_(client_.Crypto().RSA())
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
    {
    }

#if OT_CRYPTO_WITH_BIP32
    static OTAsymmetricKey get_hd_key(
        const ot::api::client::Manager& api,
        const std::string& fingerprint,
        const EcdsaCurve& curve,
        const std::uint32_t index = 0)
    {
        auto reason = api.Factory().PasswordPrompt(__FUNCTION__);
        std::string id{fingerprint};

        return OTAsymmetricKey{
            api.Seeds()
                .GetHDKey(
                    id,
                    curve,
                    {HDIndex{Bip43Purpose::NYM, Bip32Child::HARDENED},
                     HDIndex{0, Bip32Child::HARDENED},
                     HDIndex{0, Bip32Child::HARDENED},
                     HDIndex{index, Bip32Child::HARDENED},
                     HDIndex {
                         Bip32Child::SIGN_KEY,
                         Bip32Child::HARDENED
                     }},
                    reason)
                .release()};
    }
#endif  // OT_CRYPTO_WITH_BIP32
    [[maybe_unused]] static OTAsymmetricKey get_key(
        const ot::api::client::Manager& api,
        const EcdsaCurve curve)
    {
        const auto reason = api.Factory().PasswordPrompt(__FUNCTION__);

        if (EcdsaCurve::secp256k1 == curve) {
            auto params = NymParameters{NymParameterType::secp256k1};

            return api.Factory().AsymmetricKey(params, reason);
        } else if (EcdsaCurve::ed25519 == curve) {
            auto params = NymParameters{NymParameterType::ed25519};

            return api.Factory().AsymmetricKey(params, reason);
        } else {
#if OT_CRYPTO_SUPPORTED_KEY_RSA
            auto params = NymParameters{2048};

            return api.Factory().AsymmetricKey(params, reason);
#else
            OT_FAIL;
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
        }
    }

    [[maybe_unused]] bool test_dh(
        const crypto::AsymmetricProvider& lib,
        const crypto::key::Asymmetric& keyOne,
        const crypto::key::Asymmetric& keyTwo,
        const ot::Data& expected)
    {
        constexpr auto style = ot::crypto::SecretStyle::Default;
        auto reason = client_.Factory().PasswordPrompt(__FUNCTION__);
        auto secret1 = client_.Factory().Secret(0);
        auto secret2 = client_.Factory().Secret(0);
        auto output = lib.SharedSecret(keyOne, keyTwo, style, reason, secret1);

        if (false == output) { return output; }

        output = lib.SharedSecret(keyTwo, keyOne, style, reason, secret2);

        if (false == output) { return output; }

        EXPECT_EQ(secret1, secret2);

        if (0 < expected.size()) {
            EXPECT_EQ(secret1->Bytes(), expected.Bytes());

            output &= (secret1->Bytes() == expected.Bytes());
        }

        return output;
    }

    [[maybe_unused]] bool test_signature(
        const Data& plaintext,
        const crypto::AsymmetricProvider& lib,
        const crypto::key::Asymmetric& key,
        const crypto::HashType hash)
    {
        auto reason = client_.Factory().PasswordPrompt(__FUNCTION__);
        auto sig = Data::Factory();
        const auto haveSig = lib.Sign(
            client_, plaintext.Bytes(), key, hash, sig->WriteInto(), reason);

        if (false == haveSig) { return false; }

        const auto verified = lib.Verify(plaintext, key, sig, hash);

        return haveSig && verified;
    }

    [[maybe_unused]] bool bad_signature(
        const crypto::AsymmetricProvider& lib,
        const crypto::key::Asymmetric& key,
        const crypto::HashType hash)
    {
        auto reason = client_.Factory().PasswordPrompt(__FUNCTION__);
        auto sig = Data::Factory();
        lib.Sign(
            client_, plaintext_1->Bytes(), key, hash, sig->WriteInto(), reason);
        const auto verified = lib.Verify(plaintext_2, key, sig, hash);

        return !verified;
    }
};

#if OT_CRYPTO_SUPPORTED_KEY_RSA
TEST_F(Test_Signatures, RSA_unsupported_hash)
{
    EXPECT_FALSE(test_signature(plaintext_1, openssl_, rsa_, blake160_));
    EXPECT_FALSE(test_signature(plaintext_1, openssl_, rsa_, blake256_));
    EXPECT_FALSE(test_signature(plaintext_1, openssl_, rsa_, blake512_));
}

TEST_F(Test_Signatures, RSA_detect_invalid_signature)
{
    EXPECT_TRUE(bad_signature(openssl_, rsa_, sha256_));
    EXPECT_TRUE(bad_signature(openssl_, rsa_, sha512_));
    EXPECT_TRUE(bad_signature(openssl_, rsa_, ripemd160_));
}

TEST_F(Test_Signatures, RSA_supported_hashes)
{
    EXPECT_TRUE(test_signature(plaintext_1, openssl_, rsa_, sha256_));
    EXPECT_TRUE(test_signature(plaintext_1, openssl_, rsa_, sha512_));
    EXPECT_TRUE(test_signature(plaintext_1, openssl_, rsa_, ripemd160_));
}

TEST_F(Test_Signatures, RSA_DH)
{
    const auto expected = client_.Factory().Data();

    // RSA does not use the same key for encryption/signing and DH so this
    // test should fail
    EXPECT_FALSE(test_dh(openssl_, rsa_, rsa_2_, expected));
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA

#if OT_CRYPTO_SUPPORTED_KEY_ED25519
TEST_F(Test_Signatures, Ed25519_unsupported_hash)
{
#if OT_CRYPTO_WITH_BIP32
    EXPECT_FALSE(test_signature(plaintext_1, ed25519_, ed_hd_, sha256_));
    EXPECT_FALSE(test_signature(plaintext_1, ed25519_, ed_hd_, sha512_));
    EXPECT_FALSE(test_signature(plaintext_1, ed25519_, ed_hd_, ripemd160_));
    EXPECT_FALSE(test_signature(plaintext_1, ed25519_, ed_hd_, blake160_));
    EXPECT_FALSE(test_signature(plaintext_1, ed25519_, ed_hd_, blake512_));
#endif  // OT_CRYPTO_WITH_BIP32

    EXPECT_FALSE(test_signature(plaintext_1, ed25519_, ed_, sha256_));
    EXPECT_FALSE(test_signature(plaintext_1, ed25519_, ed_, sha512_));
    EXPECT_FALSE(test_signature(plaintext_1, ed25519_, ed_, ripemd160_));
    EXPECT_FALSE(test_signature(plaintext_1, ed25519_, ed_, blake160_));
    EXPECT_FALSE(test_signature(plaintext_1, ed25519_, ed_, blake512_));
}

TEST_F(Test_Signatures, Ed25519_detect_invalid_signature)
{
#if OT_CRYPTO_WITH_BIP32
    EXPECT_TRUE(bad_signature(ed25519_, ed_hd_, blake256_));
#endif  // OT_CRYPTO_WITH_BIP32
    EXPECT_TRUE(bad_signature(ed25519_, ed_, blake256_));
}

TEST_F(Test_Signatures, Ed25519_supported_hashes)
{
#if OT_CRYPTO_WITH_BIP32
    EXPECT_TRUE(test_signature(plaintext_1, ed25519_, ed_hd_, blake256_));
#endif  // OT_CRYPTO_WITH_BIP32
    EXPECT_TRUE(test_signature(plaintext_1, ed25519_, ed_, blake256_));
}

TEST_F(Test_Signatures, Ed25519_ECDH)
{
    const auto expected = client_.Factory().Data();

    EXPECT_TRUE(test_dh(ed25519_, ed_, ed_2_, expected));
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_ED25519

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
TEST_F(Test_Signatures, Secp256k1_detect_invalid_signature)
{
#if OT_CRYPTO_WITH_BIP32
    EXPECT_TRUE(bad_signature(secp256k1_, secp_hd_, sha256_));
    EXPECT_TRUE(bad_signature(secp256k1_, secp_hd_, sha512_));
    EXPECT_TRUE(bad_signature(secp256k1_, secp_hd_, blake160_));
    EXPECT_TRUE(bad_signature(secp256k1_, secp_hd_, blake256_));
    EXPECT_TRUE(bad_signature(secp256k1_, secp_hd_, blake512_));
    EXPECT_TRUE(bad_signature(secp256k1_, secp_hd_, ripemd160_));
#endif  // OT_CRYPTO_WITH_BIP32
    EXPECT_TRUE(bad_signature(secp256k1_, secp_, sha256_));
    EXPECT_TRUE(bad_signature(secp256k1_, secp_, sha512_));
    EXPECT_TRUE(bad_signature(secp256k1_, secp_, blake160_));
    EXPECT_TRUE(bad_signature(secp256k1_, secp_, blake256_));
    EXPECT_TRUE(bad_signature(secp256k1_, secp_, blake512_));
    EXPECT_TRUE(bad_signature(secp256k1_, secp_, ripemd160_));
}

TEST_F(Test_Signatures, Secp256k1_supported_hashes)
{
#if OT_CRYPTO_WITH_BIP32
    EXPECT_TRUE(test_signature(plaintext_1, secp256k1_, secp_hd_, sha256_));
    EXPECT_TRUE(test_signature(plaintext_1, secp256k1_, secp_hd_, sha512_));
    EXPECT_TRUE(test_signature(plaintext_1, secp256k1_, secp_hd_, blake160_));
    EXPECT_TRUE(test_signature(plaintext_1, secp256k1_, secp_hd_, blake256_));
    EXPECT_TRUE(test_signature(plaintext_1, secp256k1_, secp_hd_, blake512_));
    EXPECT_TRUE(test_signature(plaintext_1, secp256k1_, secp_hd_, ripemd160_));
#endif  // OT_CRYPTO_WITH_BIP32

    EXPECT_TRUE(test_signature(plaintext_1, secp256k1_, secp_, sha256_));
    EXPECT_TRUE(test_signature(plaintext_1, secp256k1_, secp_, sha512_));
    EXPECT_TRUE(test_signature(plaintext_1, secp256k1_, secp_, blake256_));
    EXPECT_TRUE(test_signature(plaintext_1, secp256k1_, secp_, blake512_));
    EXPECT_TRUE(test_signature(plaintext_1, secp256k1_, secp_, blake160_));
    EXPECT_TRUE(test_signature(plaintext_1, secp256k1_, secp_, ripemd160_));
}

TEST_F(Test_Signatures, Secp256k1_ECDH)
{
    constexpr auto hex
    {
#if OT_CRYPTO_WITH_BIP32
        "a3f8385b75ab90df6f6aae81044730ec526104033f8aac662dde1f1b40453e58"
#else
        ""
#endif  // OT_CRYPTO_WITH_BIP32
    };

    const auto expected = client_.Factory().Data(hex, ot::StringStyle::Hex);

    EXPECT_TRUE(test_dh(secp256k1_, secp_, secp_2_, expected));
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
}  // namespace
