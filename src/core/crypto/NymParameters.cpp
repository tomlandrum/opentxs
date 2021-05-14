// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                           // IWYU pragma: associated
#include "1_Internal.hpp"                         // IWYU pragma: associated
#include "opentxs/core/crypto/NymParameters.hpp"  // IWYU pragma: associated

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>

#include "internal/crypto/key/Factory.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Primitives.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/crypto/Language.hpp"
#include "opentxs/crypto/SeedStrength.hpp"
#include "opentxs/crypto/SeedStyle.hpp"
#include "opentxs/crypto/key/asymmetric/Algorithm.hpp"
#include "opentxs/identity/SourceProofType.hpp"
#include "opentxs/identity/SourceType.hpp"
#include "opentxs/protobuf/ContactData.pb.h"
#include "opentxs/protobuf/VerificationSet.pb.h"
#include "util/Container.hpp"

namespace opentxs
{
const std::map<crypto::key::asymmetric::Algorithm, NymParameterType> key_to_nym_
{
#if OT_CRYPTO_SUPPORTED_KEY_RSA
    {crypto::key::asymmetric::Algorithm::Legacy, NymParameterType::rsa},
#endif
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
        {crypto::key::asymmetric::Algorithm::Secp256k1,
         NymParameterType::secp256k1},
#endif
#if OT_CRYPTO_SUPPORTED_KEY_ED25519
        {crypto::key::asymmetric::Algorithm::ED25519,
         NymParameterType::ed25519},
#endif
};
const auto nym_to_key_{reverse_map(key_to_nym_)};

struct NymParameters::Imp {
    const NymParameterType nymType_;
    const identity::CredentialType credentialType_;
    const identity::SourceType sourceType_;
    const identity::SourceProofType sourceProofType_;
    std::shared_ptr<proto::ContactData> contact_data_;
    std::shared_ptr<proto::VerificationSet> verification_set_;
    std::uint8_t payment_code_version_;
#if OT_CRYPTO_WITH_BIP32
    crypto::SeedStyle seed_style_;
    crypto::Language seed_language_;
    crypto::SeedStrength seed_strength_;
    OTSecret entropy_;
    std::string seed_;
    Bip32Index nym_;
    Bip32Index credset_;
    Bip32Index cred_index_;
    bool default_;
    bool use_auto_index_;
#endif
#if OT_CRYPTO_SUPPORTED_KEY_RSA
    std::int32_t nBits_;
    Space params_;
#endif
    OTKeypair source_keypair_;

    Imp(const NymParameterType type,
        const identity::CredentialType credential,
        const identity::SourceType source,
        const std::uint8_t pcVersion) noexcept
        : nymType_(type)
        , credentialType_(
              (NymParameterType::rsa == nymType_)
                  ? identity::CredentialType::Legacy
                  : credential)
        , sourceType_(
              (NymParameterType::rsa == nymType_) ? identity::SourceType::PubKey
                                                  : source)
        , sourceProofType_(
              (identity::SourceType::Bip47 == sourceType_)
                  ? identity::SourceProofType::Signature
                  : identity::SourceProofType::SelfSignature)
        , contact_data_(nullptr)
        , verification_set_(nullptr)
        , payment_code_version_(
              (0 == pcVersion) ? PaymentCode::DefaultVersion : pcVersion)
#if OT_CRYPTO_WITH_BIP32
        , seed_style_(crypto::SeedStyle::BIP39)
        , seed_language_(crypto::Language::en)
        , seed_strength_(crypto::SeedStrength::TwentyFour)
        , entropy_(Context().Factory().Secret(0))
        , seed_("")
        , nym_(0)
        , credset_(0)
        , cred_index_(0)
        , default_(true)
        , use_auto_index_(true)
#endif  // OT_CRYPTO_WITH_BIP32
#if OT_CRYPTO_SUPPORTED_KEY_RSA
        , nBits_(1024)
        , params_()
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
        , source_keypair_(factory::Keypair())
    {
    }

    Imp(const Imp& rhs) noexcept
        : Imp(rhs.nymType_,
              rhs.credentialType_,
              rhs.sourceType_,
              rhs.payment_code_version_)
    {
        contact_data_ = rhs.contact_data_;
        verification_set_ = rhs.verification_set_;
#if OT_CRYPTO_WITH_BIP32
        entropy_ = rhs.entropy_;
        seed_ = rhs.seed_;
        nym_ = rhs.nym_;
        credset_ = rhs.credset_;
        cred_index_ = rhs.cred_index_;
        default_ = rhs.default_;
        use_auto_index_ = rhs.use_auto_index_;
#endif
#if OT_CRYPTO_SUPPORTED_KEY_RSA
        nBits_ = rhs.nBits_;
        params_ = rhs.params_;
#endif
    }
};

NymParameters::NymParameters(
    const NymParameterType type,
    const identity::CredentialType credential,
    const identity::SourceType source,
    const std::uint8_t pcVersion) noexcept
    : imp_(std::make_unique<Imp>(type, credential, source, pcVersion))
{
    OT_ASSERT(imp_);
}

NymParameters::NymParameters(
    crypto::key::asymmetric::Algorithm key,
    identity::CredentialType credential,
    const identity::SourceType source,
    const std::uint8_t pcVersion) noexcept
    : NymParameters(key_to_nym_.at(key), credential, source, pcVersion)
{
}

#if OT_CRYPTO_SUPPORTED_KEY_RSA
NymParameters::NymParameters(const std::int32_t keySize) noexcept
    : NymParameters(NymParameterType::rsa, identity::CredentialType::Legacy)
{
    imp_->nBits_ = keySize;
}
#endif

NymParameters::NymParameters(
    [[maybe_unused]] const std::string& seedID,
    [[maybe_unused]] const int index,
    const std::uint8_t pcVersion) noexcept
    : NymParameters()
{
#if OT_CRYPTO_WITH_BIP32
    if (0 < seedID.size()) { SetSeed(seedID); }

    if (index >= 0) { SetNym(static_cast<Bip32Index>(index)); }
#endif  // OT_CRYPTO_WITH_BIP32

    if (0 != pcVersion) { SetPaymentCodeVersion(pcVersion); }
}

NymParameters::NymParameters(const NymParameters& rhs) noexcept
    : imp_(std::make_unique<Imp>(*rhs.imp_))
{
}

auto NymParameters::Algorithm() const noexcept
    -> crypto::key::asymmetric::Algorithm
{
    try {
        return nym_to_key_.at(imp_->nymType_);
    } catch (...) {
        return crypto::key::asymmetric::Algorithm::Error;
    }
}

auto NymParameters::ChangeType(const NymParameterType type) const noexcept
    -> NymParameters
{
    auto output{*this};
    const_cast<NymParameterType&>(output.imp_->nymType_) = type;

    if (NymParameterType::rsa == output.imp_->nymType_) {
        const_cast<identity::CredentialType&>(output.imp_->credentialType_) =
            identity::CredentialType::Legacy;
        const_cast<identity::SourceType&>(output.imp_->sourceType_) =
            identity::SourceType::PubKey;
        const_cast<identity::SourceProofType&>(output.imp_->sourceProofType_) =
            identity::SourceProofType::SelfSignature;
    }

    return output;
}

auto NymParameters::credentialType() const noexcept -> identity::CredentialType
{
    return imp_->credentialType_;
}

#if OT_CRYPTO_WITH_BIP32
auto NymParameters::CredIndex() const noexcept -> Bip32Index
{
    return imp_->cred_index_;
}
auto NymParameters::Credset() const noexcept -> Bip32Index
{
    return imp_->credset_;
}
auto NymParameters::Default() const noexcept -> bool { return imp_->default_; }
#endif  // OT_CRYPTO_WITH_BIP32
#if OT_CRYPTO_SUPPORTED_KEY_RSA
auto NymParameters::DHParams() const noexcept -> ReadView
{
    return reader(imp_->params_);
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
#if OT_CRYPTO_WITH_BIP32
auto NymParameters::Entropy() const noexcept -> const Secret&
{
    return imp_->entropy_;
}
#endif  // OT_CRYPTO_WITH_BIP32
auto NymParameters::GetContactData(
    proto::ContactData& serialized) const noexcept -> bool
{
    if (false == bool{imp_->contact_data_}) { return false; }

    serialized = *imp_->contact_data_;

    return true;
}

auto NymParameters::GetVerificationSet(
    proto::VerificationSet& serialized) const noexcept -> bool
{
    if (false == bool{imp_->verification_set_}) { return false; }

    serialized = *imp_->verification_set_;

    return true;
}

auto NymParameters::Keypair() const noexcept -> const crypto::key::Keypair&
{
    return imp_->source_keypair_;
}
#if OT_CRYPTO_SUPPORTED_KEY_RSA
auto NymParameters::keySize() const noexcept -> std::int32_t
{
    return imp_->nBits_;
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA

#if OT_CRYPTO_WITH_BIP32
auto NymParameters::Nym() const noexcept -> Bip32Index { return imp_->nym_; }
#endif  // OT_CRYPTO_WITH_BIP32

auto NymParameters::nymParameterType() const noexcept -> NymParameterType
{
    return imp_->nymType_;
}

auto NymParameters::PaymentCodeVersion() const noexcept -> std::uint8_t
{
    return imp_->payment_code_version_;
}

#if OT_CRYPTO_WITH_BIP32
auto NymParameters::Seed() const noexcept -> std::string { return imp_->seed_; }
auto NymParameters::SeedLanguage() const noexcept -> crypto::Language
{
    return imp_->seed_language_;
}
auto NymParameters::SeedStrength() const noexcept -> crypto::SeedStrength
{
    return imp_->seed_strength_;
}
auto NymParameters::SeedStyle() const noexcept -> crypto::SeedStyle
{
    return imp_->seed_style_;
}
#endif  // OT_CRYPTO_WITH_BIP32
auto NymParameters::SourceProofType() const noexcept
    -> identity::SourceProofType
{
    return imp_->sourceProofType_;
}
auto NymParameters::SourceType() const noexcept -> identity::SourceType
{
    return imp_->sourceType_;
}
#if OT_CRYPTO_WITH_BIP32
auto NymParameters::UseAutoIndex() const noexcept -> bool
{
    return imp_->use_auto_index_;
}
#endif  // OT_CRYPTO_WITH_BIP32
auto NymParameters::Keypair() noexcept -> OTKeypair&
{
    return imp_->source_keypair_;
}
auto NymParameters::SetContactData(
    const proto::ContactData& contactData) noexcept -> void
{
    imp_->contact_data_.reset(new proto::ContactData(contactData));
}

#if OT_CRYPTO_WITH_BIP32
auto NymParameters::SetCredIndex(const Bip32Index path) noexcept -> void
{
    imp_->cred_index_ = path;
}
auto NymParameters::SetCredset(const Bip32Index path) noexcept -> void
{
    imp_->credset_ = path;
}
auto NymParameters::SetDefault(const bool in) noexcept -> void
{
    imp_->default_ = in;
}
#endif  // OT_CRYPTO_WITH_BIP32

#if OT_CRYPTO_SUPPORTED_KEY_RSA
auto NymParameters::SetDHParams(const ReadView bytes) noexcept -> void
{
    auto start = reinterpret_cast<const std::byte*>(bytes.data());
    auto end = start + bytes.size();

    imp_->params_.assign(start, end);
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA

#if OT_CRYPTO_WITH_BIP32
auto NymParameters::SetEntropy(const Secret& entropy) noexcept -> void
{
    imp_->entropy_ = entropy;
}
#endif  // OT_CRYPTO_WITH_BIP32

#if OT_CRYPTO_SUPPORTED_KEY_RSA
auto NymParameters::setKeySize(std::int32_t keySize) noexcept -> void
{
    imp_->nBits_ = keySize;
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA

#if OT_CRYPTO_WITH_BIP32
auto NymParameters::SetNym(const Bip32Index path) noexcept -> void
{
    imp_->nym_ = path;
    imp_->use_auto_index_ = false;
}
#endif

auto NymParameters::SetPaymentCodeVersion(const std::uint8_t version) noexcept
    -> void
{
    imp_->payment_code_version_ = version;
}

#if OT_CRYPTO_WITH_BIP32
auto NymParameters::SetSeed(const std::string& seed) noexcept -> void
{
    imp_->seed_ = seed;
}
auto NymParameters::SetSeedLanguage(const crypto::Language lang) noexcept
    -> void
{
    imp_->seed_language_ = lang;
}
auto NymParameters::SetSeedStrength(const crypto::SeedStrength type) noexcept
    -> void
{
    imp_->seed_strength_ = type;
}
auto NymParameters::SetSeedStyle(const crypto::SeedStyle type) noexcept -> void
{
    imp_->seed_style_ = type;
}
#endif  // OT_CRYPTO_WITH_BIP32

#if OT_CRYPTO_WITH_BIP32
auto NymParameters::SetUseAutoIndex(const bool use) noexcept -> void
{
    imp_->use_auto_index_ = use;
}
#endif

auto NymParameters::SetVerificationSet(
    const proto::VerificationSet& verificationSet) noexcept -> void
{
    imp_->verification_set_.reset(new proto::VerificationSet(verificationSet));
}

NymParameters::~NymParameters() = default;
}  // namespace opentxs
