/************************************************************
 *
 *                 OPEN TRANSACTIONS
 *
 *       Financial Cryptography and Digital Cash
 *       Library, Protocol, API, Server, CLI, GUI
 *
 *       -- Anonymous Numbered Accounts.
 *       -- Untraceable Digital Cash.
 *       -- Triple-Signed Receipts.
 *       -- Cheques, Vouchers, Transfers, Inboxes.
 *       -- Basket Currencies, Markets, Payment Plans.
 *       -- Signed, XML, Ricardian-style Contracts.
 *       -- Scripted smart contracts.
 *
 *  EMAIL:
 *  fellowtraveler@opentransactions.org
 *
 *  WEBSITE:
 *  http://www.opentransactions.org/
 *
 *  -----------------------------------------------------
 *
 *   LICENSE:
 *   This Source Code Form is subject to the terms of the
 *   Mozilla Public License, v. 2.0. If a copy of the MPL
 *   was not distributed with this file, You can obtain one
 *   at http://mozilla.org/MPL/2.0/.
 *
 *   DISCLAIMER:
 *   This program is distributed in the hope that it will
 *   be useful, but WITHOUT ANY WARRANTY; without even the
 *   implied warranty of MERCHANTABILITY or FITNESS FOR A
 *   PARTICULAR PURPOSE.  See the Mozilla Public License
 *   for more details.
 *
 ************************************************************/

#pragma once

#if OT_WITH_BLOCKCHAIN
namespace opentxs::crypto::implementation
{
class Bitcoin final : virtual public crypto::Bitcoin,
                      virtual public EncodingProvider
#if OT_CRYPTO_WITH_BIP39
    ,
                      virtual public crypto::Bip39
#endif
#if OT_CRYPTO_WITH_BIP32
    ,
                      public Bip32
#endif
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    ,
                      public AsymmetricProvider,
                      public EcdsaProvider
#endif
{
public:
#if OT_CRYPTO_WITH_BIP32
    std::shared_ptr<proto::AsymmetricKey> GetChild(
        const proto::AsymmetricKey& parent,
        const std::uint32_t index) const override;
    std::shared_ptr<proto::AsymmetricKey> GetHDKey(
        const EcdsaCurve& curve,
        const OTPassword& seed,
        proto::HDPath& path) const override;
    bool RandomKeypair(OTPassword& privateKey, Data& publicKey) const override;
    std::string SeedToFingerprint(
        const EcdsaCurve& curve,
        const OTPassword& seed) const override;
    std::shared_ptr<proto::AsymmetricKey> SeedToPrivateKey(
        const EcdsaCurve& curve,
        const OTPassword& seed) const override;
#endif

#if OT_CRYPTO_WITH_BIP39
    bool SeedToWords(const OTPassword& seed, OTPassword& words) const override;
    void WordsToSeed(
        const OTPassword& words,
        OTPassword& seed,
        const OTPassword& passphrase) const override;
#endif

    std::string Base58CheckEncode(
        const std::uint8_t* inputStart,
        const std::size_t& inputSize) const override;
    bool Base58CheckDecode(const std::string&& input, RawData& output)
        const override;
    bool RIPEMD160(
        const std::uint8_t* input,
        const size_t inputSize,
        std::uint8_t* output) const override;

    bool Sign(
        const Data& plaintext,
        const key::Asymmetric& theKey,
        const proto::HashType hashType,
        Data& signature,  // output
        const OTPasswordData* pPWData = nullptr,
        const OTPassword* exportPassword = nullptr) const override;
    bool Verify(
        const Data& plaintext,
        const key::Asymmetric& theKey,
        const Data& signature,
        const proto::HashType hashType,
        const OTPasswordData* pPWData = nullptr) const override;

    ~Bitcoin() = default;

private:
    friend opentxs::Factory;

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    bool ECDH(
        const Data& publicKey,
        const OTPassword& privateKey,
        OTPassword& secret) const override;
    bool ScalarBaseMultiply(const OTPassword& privateKey, Data& publicKey)
        const override;
#endif

    Bitcoin(const api::Crypto& crypto);
    Bitcoin() = delete;
    Bitcoin(const Bitcoin&) = delete;
    Bitcoin(Bitcoin&&) = delete;
    Bitcoin& operator=(const Bitcoin&) = delete;
    Bitcoin& operator=(Bitcoin&&) = delete;
};
}  // namespace opentxs::crypto::implementation
#endif  // OT_WITH_BLOCKCHAIN
