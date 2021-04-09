// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CRYPTO_NYMPARAMETERS_HPP
#define OPENTXS_CORE_CRYPTO_NYMPARAMETERS_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"
// IWYU pragma: no_include "opentxs/crypto/Language.hpp"
// IWYU pragma: no_include "opentxs/crypto/SeedStrength.hpp"
// IWYU pragma: no_include "opentxs/crypto/SeedStyle.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>
#include <string>

#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#if OT_CRYPTO_WITH_BIP32
#include "opentxs/core/Secret.hpp"
#endif  // OT_CRYPTO_WITH_BIP32
#include "opentxs/crypto/Types.hpp"
#include "opentxs/crypto/key/Keypair.hpp"
#include "opentxs/crypto/key/Types.hpp"
#include "opentxs/identity/CredentialType.hpp"
#include "opentxs/identity/credential/Base.hpp"
#include "opentxs/identity/SourceType.hpp"
#include "opentxs/identity/Types.hpp"

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
class NymParameters
{
public:
    OPENTXS_EXPORT crypto::key::asymmetric::Algorithm Algorithm()
        const noexcept;
    OPENTXS_EXPORT NymParameters
    ChangeType(const NymParameterType type) const noexcept;
    OPENTXS_EXPORT std::shared_ptr<proto::ContactData> ContactData()
        const noexcept;
    OPENTXS_EXPORT identity::CredentialType credentialType() const noexcept;
#if OT_CRYPTO_WITH_BIP32
    OPENTXS_EXPORT Bip32Index CredIndex() const noexcept;
    OPENTXS_EXPORT Bip32Index Credset() const noexcept;
    OPENTXS_EXPORT bool Default() const noexcept;
#endif  // OT_CRYPTO_WITH_BIP32
#if OT_CRYPTO_SUPPORTED_KEY_RSA
    OPENTXS_EXPORT ReadView DHParams() const noexcept;
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
#if OT_CRYPTO_WITH_BIP32
    OPENTXS_EXPORT const Secret& Entropy() const noexcept;
#endif  // OT_CRYPTO_WITH_BIP32
    OPENTXS_EXPORT const crypto::key::Keypair& Keypair() const noexcept;
#if OT_CRYPTO_SUPPORTED_KEY_RSA
    OPENTXS_EXPORT std::int32_t keySize() const noexcept;
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
#if OT_CRYPTO_WITH_BIP32
    OPENTXS_EXPORT Bip32Index Nym() const noexcept;
#endif  // OT_CRYPTO_WITH_BIP32
    OPENTXS_EXPORT NymParameterType nymParameterType() const noexcept;
    OPENTXS_EXPORT std::uint8_t PaymentCodeVersion() const noexcept;
#if OT_CRYPTO_WITH_BIP32
    OPENTXS_EXPORT std::string Seed() const noexcept;
    OPENTXS_EXPORT crypto::Language SeedLanguage() const noexcept;
    OPENTXS_EXPORT crypto::SeedStrength SeedStrength() const noexcept;
    OPENTXS_EXPORT crypto::SeedStyle SeedStyle() const noexcept;
#endif  // OT_CRYPTO_WITH_BIP32
    OPENTXS_EXPORT identity::SourceProofType SourceProofType() const noexcept;
    OPENTXS_EXPORT identity::SourceType SourceType() const noexcept;
#if OT_CRYPTO_WITH_BIP32
    OPENTXS_EXPORT bool UseAutoIndex() const noexcept;
#endif  // OT_CRYPTO_WITH_BIP32
    OPENTXS_EXPORT std::shared_ptr<proto::VerificationSet> VerificationSet()
        const noexcept;

    OPENTXS_EXPORT OTKeypair& Keypair() noexcept;
    OPENTXS_EXPORT void SetContactData(
        const proto::ContactData& contactData) noexcept;
#if OT_CRYPTO_WITH_BIP32
    OPENTXS_EXPORT void SetCredIndex(const Bip32Index path) noexcept;
    OPENTXS_EXPORT void SetCredset(const Bip32Index path) noexcept;
    OPENTXS_EXPORT void SetDefault(const bool in) noexcept;
    OPENTXS_EXPORT void SetEntropy(const Secret& entropy) noexcept;
#endif  // OT_CRYPTO_WITH_BIP32
#if OT_CRYPTO_SUPPORTED_KEY_RSA
    OPENTXS_EXPORT void setKeySize(std::int32_t keySize) noexcept;
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
#if OT_CRYPTO_WITH_BIP32
    OPENTXS_EXPORT void SetNym(const Bip32Index path) noexcept;
#endif  // OT_CRYPTO_WITH_BIP32
#if OT_CRYPTO_SUPPORTED_KEY_RSA
    OPENTXS_EXPORT void SetDHParams(const ReadView bytes) noexcept;
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
    OPENTXS_EXPORT void SetPaymentCodeVersion(
        const std::uint8_t version) noexcept;
#if OT_CRYPTO_WITH_BIP32
    OPENTXS_EXPORT void SetSeed(const std::string& seed) noexcept;
    OPENTXS_EXPORT void SetSeedLanguage(const crypto::Language lang) noexcept;
    OPENTXS_EXPORT void SetSeedStrength(
        const crypto::SeedStrength value) noexcept;
    OPENTXS_EXPORT void SetSeedStyle(const crypto::SeedStyle type) noexcept;
#endif  // OT_CRYPTO_WITH_BIP32
#if OT_CRYPTO_WITH_BIP32
    OPENTXS_EXPORT void SetUseAutoIndex(const bool use) noexcept;
#endif
    OPENTXS_EXPORT void SetVerificationSet(
        const proto::VerificationSet& verificationSet) noexcept;

    OPENTXS_EXPORT NymParameters(
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
    OPENTXS_EXPORT NymParameters(
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
    OPENTXS_EXPORT explicit NymParameters(const std::int32_t keySize) noexcept;
#endif
    OPENTXS_EXPORT NymParameters(
        const std::string& seedID,
        const int index,
        const std::uint8_t pcVersion = 0) noexcept;
    OPENTXS_EXPORT NymParameters(const NymParameters&) noexcept;

    OPENTXS_EXPORT ~NymParameters();

private:
    struct Imp;

    std::unique_ptr<Imp> imp_;
};
}  // namespace opentxs
#endif
