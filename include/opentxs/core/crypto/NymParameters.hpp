// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CRYPTO_NYMPARAMETERS_HPP
#define OPENTXS_CORE_CRYPTO_NYMPARAMETERS_HPP

// IWYU pragma: no_include "opentxs/crypto/Language.hpp"
// IWYU pragma: no_include "opentxs/crypto/SeedStrength.hpp"
// IWYU pragma: no_include "opentxs/crypto/SeedStyle.hpp"
// IWYU pragma: no_include "opentxs/crypto/key/asymmetric/Algorithm.hpp"
// IWYU pragma: no_include "opentxs/identity/SourceProofType.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>
#include <string>

#include "opentxs/Bytes.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/crypto/key/Keypair.hpp"
#include "opentxs/crypto/key/Types.hpp"
#include "opentxs/identity/CredentialType.hpp"
#include "opentxs/identity/SourceType.hpp"
#include "opentxs/identity/Types.hpp"
#include "opentxs/identity/credential/Base.hpp"

namespace opentxs
{
namespace proto
{
class ContactData;
class VerificationSet;
}  // namespace proto

class Secret;
}  // namespace opentxs

namespace opentxs
{
class OPENTXS_EXPORT NymParameters
{
public:
    crypto::key::asymmetric::Algorithm Algorithm() const noexcept;
    NymParameters ChangeType(const NymParameterType type) const noexcept;
    identity::CredentialType credentialType() const noexcept;
#if OT_CRYPTO_WITH_BIP32
    Bip32Index CredIndex() const noexcept;
    Bip32Index Credset() const noexcept;
    bool Default() const noexcept;
#endif  // OT_CRYPTO_WITH_BIP32
#if OT_CRYPTO_SUPPORTED_KEY_RSA
    ReadView DHParams() const noexcept;
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
#if OT_CRYPTO_WITH_BIP32
    const Secret& Entropy() const noexcept;
#endif  // OT_CRYPTO_WITH_BIP32
    const crypto::key::Keypair& Keypair() const noexcept;
#if OT_CRYPTO_SUPPORTED_KEY_RSA
    std::int32_t keySize() const noexcept;
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
#if OT_CRYPTO_WITH_BIP32
    Bip32Index Nym() const noexcept;
#endif  // OT_CRYPTO_WITH_BIP32
    NymParameterType nymParameterType() const noexcept;
    std::uint8_t PaymentCodeVersion() const noexcept;
#if OT_CRYPTO_WITH_BIP32
    std::string Seed() const noexcept;
    crypto::Language SeedLanguage() const noexcept;
    crypto::SeedStrength SeedStrength() const noexcept;
    crypto::SeedStyle SeedStyle() const noexcept;
#endif  // OT_CRYPTO_WITH_BIP32
    OPENTXS_NO_EXPORT bool Serialize(
        proto::ContactData& serialized) const noexcept;
    OPENTXS_NO_EXPORT bool Serialize(
        proto::VerificationSet& serialized) const noexcept;
    identity::SourceProofType SourceProofType() const noexcept;
    identity::SourceType SourceType() const noexcept;
#if OT_CRYPTO_WITH_BIP32
    bool UseAutoIndex() const noexcept;
#endif  // OT_CRYPTO_WITH_BIP32

    OTKeypair& Keypair() noexcept;
    OPENTXS_NO_EXPORT void SetContactData(
        const proto::ContactData& contactData) noexcept;
#if OT_CRYPTO_WITH_BIP32
    void SetCredIndex(const Bip32Index path) noexcept;
    void SetCredset(const Bip32Index path) noexcept;
    void SetDefault(const bool in) noexcept;
    void SetEntropy(const Secret& entropy) noexcept;
#endif  // OT_CRYPTO_WITH_BIP32
#if OT_CRYPTO_SUPPORTED_KEY_RSA
    void setKeySize(std::int32_t keySize) noexcept;
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
#if OT_CRYPTO_WITH_BIP32
    void SetNym(const Bip32Index path) noexcept;
#endif  // OT_CRYPTO_WITH_BIP32
#if OT_CRYPTO_SUPPORTED_KEY_RSA
    void SetDHParams(const ReadView bytes) noexcept;
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
    void SetPaymentCodeVersion(const std::uint8_t version) noexcept;
#if OT_CRYPTO_WITH_BIP32
    void SetSeed(const std::string& seed) noexcept;
    void SetSeedLanguage(const crypto::Language lang) noexcept;
    void SetSeedStrength(const crypto::SeedStrength value) noexcept;
    void SetSeedStyle(const crypto::SeedStyle type) noexcept;
#endif  // OT_CRYPTO_WITH_BIP32
#if OT_CRYPTO_WITH_BIP32
    void SetUseAutoIndex(const bool use) noexcept;
#endif
    OPENTXS_NO_EXPORT void SetVerificationSet(
        const proto::VerificationSet& verificationSet) noexcept;

    NymParameters(
        const NymParameterType type =
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
            NymParameterType::secp256k1
#elif OT_CRYPTO_SUPPORTED_KEY_ED25519
            NymParameterType::ed25519
#elif OT_CRYPTO_SUPPORTED_KEY_RSA
            NymParameterType::rsa
#else
            NymParameterType::error
#endif
        ,
        const identity::CredentialType credential =
#if OT_CRYPTO_WITH_BIP32
            identity::CredentialType::HD
#else
            identity::CredentialType::Legacy
#endif
        ,
        const identity::SourceType source =
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
            identity::SourceType::Bip47
#else
            identity::SourceType::PubKey
#endif
        ,
        const std::uint8_t pcVersion = 0) noexcept;
    NymParameters(
        crypto::key::asymmetric::Algorithm key,
        identity::CredentialType credential =
#if OT_CRYPTO_WITH_BIP32
            identity::CredentialType::HD
#else
            identity::CredentialType::Legacy
#endif
        ,
        const identity::SourceType source =
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
            identity::SourceType::Bip47
#else
            identity::SourceType::PubKey
#endif
        ,
        const std::uint8_t pcVersion = 0) noexcept;
#if OT_CRYPTO_SUPPORTED_KEY_RSA
    explicit NymParameters(const std::int32_t keySize) noexcept;
#endif
    NymParameters(
        const std::string& seedID,
        const int index,
        const std::uint8_t pcVersion = 0) noexcept;
    NymParameters(const NymParameters&) noexcept;

    ~NymParameters();

private:
    struct Imp;

    std::unique_ptr<Imp> imp_;
};
}  // namespace opentxs
#endif
