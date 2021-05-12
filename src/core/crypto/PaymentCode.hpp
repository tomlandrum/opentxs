// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/core/crypto/PaymentCode.cpp"

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include "opentxs/Bytes.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace crypto
{
namespace key
{
class EllipticCurve;
class Secp256k1;
}  // namespace key

class EcdsaProvider;
}  // namespace crypto

namespace identity
{
namespace credential
{
class Base;
}  // namespace credential
}  // namespace identity

namespace proto
{
class Credential;
class PaymentCode;
class Signature;
}  // namespace proto

class Factory;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::implementation
{
class PaymentCode final : virtual public opentxs::PaymentCode
{
public:
    struct XpubPreimage {
        std::array<std::byte, 33> key_;
        std::array<std::byte, 32> code_;

        operator ReadView() const noexcept
        {
            return {reinterpret_cast<const char*>(this), sizeof(*this)};
        }

        auto Chaincode() const noexcept -> ReadView
        {
            return {reinterpret_cast<const char*>(code_.data()), code_.size()};
        }
        auto Key() const noexcept -> ReadView
        {
            return {reinterpret_cast<const char*>(key_.data()), key_.size()};
        }

        XpubPreimage(const ReadView key, const ReadView code) noexcept
            : key_()
            , code_()
        {
            static_assert(65 == sizeof(XpubPreimage));

            if (nullptr != key.data()) {
                std::memcpy(
                    key_.data(), key.data(), std::min(key_.size(), key.size()));
            }

            if (nullptr != code.data()) {
                std::memcpy(
                    code_.data(),
                    code.data(),
                    std::min(code_.size(), code.size()));
            }
        }
        XpubPreimage() noexcept
            : XpubPreimage({}, {})
        {
        }
    };

    struct BinaryPreimage {
        std::uint8_t version_;
        std::uint8_t features_;
        XpubPreimage xpub_;
        std::uint8_t bm_version_;
        std::uint8_t bm_stream_;
        std::array<std::byte, 11> blank_;

        operator ReadView() const noexcept
        {
            return {reinterpret_cast<const char*>(this), sizeof(*this)};
        }

        auto haveBitmessage() const noexcept -> bool
        {
            return 0 != (features_ & std::uint8_t{0x80});
        }

        BinaryPreimage(
            const VersionNumber version,
            const bool bitmessage,
            const ReadView key,
            const ReadView code,
            const std::uint8_t bmVersion,
            const std::uint8_t bmStream) noexcept
            : version_(static_cast<std::uint8_t>(version))
            , features_(bitmessage ? 0x80 : 0x00)
            , xpub_(key, code)
            , bm_version_(bmVersion)
            , bm_stream_(bmStream)
            , blank_()
        {
            static_assert(80 == sizeof(BinaryPreimage));
        }
        BinaryPreimage() noexcept
            : BinaryPreimage(0, false, {}, {}, 0, 0)
        {
        }
    };

    struct BinaryPreimage_3 {
        std::uint8_t version_;
        std::array<std::byte, 33> key_;

        operator ReadView() const noexcept
        {
            return {reinterpret_cast<const char*>(this), sizeof(*this)};
        }

        auto Key() const noexcept -> ReadView
        {
            return {reinterpret_cast<const char*>(key_.data()), key_.size()};
        }

        BinaryPreimage_3(
            const VersionNumber version,
            const ReadView key) noexcept
            : version_(version)
            , key_()
        {
            static_assert(34 == sizeof(BinaryPreimage_3));

            if (nullptr != key.data()) {
                std::memcpy(
                    key_.data(), key.data(), std::min(key_.size(), key.size()));
            }
        }
        BinaryPreimage_3() noexcept
            : BinaryPreimage_3(0, {})
        {
        }
    };

    struct Base58Preimage {
        static constexpr auto expected_prefix_ = std::byte{0x47};

        std::uint8_t prefix_;
        BinaryPreimage payload_;

        operator ReadView() const noexcept
        {
            return {reinterpret_cast<const char*>(this), sizeof(*this)};
        }

        Base58Preimage(BinaryPreimage&& data) noexcept
            : prefix_(std::to_integer<std::uint8_t>(expected_prefix_))
            , payload_(std::move(data))
        {
            static_assert(81 == sizeof(Base58Preimage));
        }
        Base58Preimage(
            const VersionNumber version,
            const bool bitmessage,
            const ReadView key,
            const ReadView code,
            const std::uint8_t bmVersion,
            const std::uint8_t bmStream) noexcept
            : Base58Preimage(BinaryPreimage{
                  version,
                  bitmessage,
                  key,
                  code,
                  bmVersion,
                  bmStream})
        {
        }
        Base58Preimage() noexcept
            : Base58Preimage(BinaryPreimage{})
        {
        }
    };

    struct Base58Preimage_3 {
        static constexpr auto expected_prefix_ = std::byte{0x22};

        std::uint8_t prefix_;
        BinaryPreimage_3 payload_;

        operator ReadView() const noexcept
        {
            return {reinterpret_cast<const char*>(this), sizeof(*this)};
        }

        Base58Preimage_3(BinaryPreimage_3&& data) noexcept
            : prefix_(std::to_integer<std::uint8_t>(expected_prefix_))
            , payload_(std::move(data))
        {
            static_assert(35 == sizeof(Base58Preimage_3));
        }
        Base58Preimage_3(
            const VersionNumber version,
            const ReadView key) noexcept
            : Base58Preimage_3(BinaryPreimage_3{version, key})
        {
        }
        Base58Preimage_3() noexcept
            : Base58Preimage_3(BinaryPreimage_3{})
        {
        }
    };

    static const std::size_t pubkey_size_;
    static const std::size_t chain_code_size_;

    operator const opentxs::crypto::key::Asymmetric&() const noexcept final;

    auto operator==(const proto::PaymentCode& rhs) const noexcept -> bool final;

    auto asBase58() const noexcept -> std::string final;
    auto Blind(
        const opentxs::PaymentCode& recipient,
        const crypto::key::EllipticCurve& privateKey,
        const ReadView outpoint,
        const AllocateOutput destination,
        const PasswordPrompt& reason) const noexcept -> bool final;
    auto BlindV3(
        const opentxs::PaymentCode& recipient,
        const crypto::key::EllipticCurve& privateKey,
        const AllocateOutput destination,
        const PasswordPrompt& reason) const noexcept -> bool final;
    auto DecodeNotificationElements(
        const std::uint8_t version,
        const Elements& elements,
        const PasswordPrompt& reason) const noexcept
        -> std::unique_ptr<opentxs::PaymentCode> final;
    auto GenerateNotificationElements(
        const opentxs::PaymentCode& recipient,
        const crypto::key::EllipticCurve& privateKey,
        const PasswordPrompt& reason) const noexcept -> Elements final;
    auto ID() const noexcept -> const identifier::Nym& final { return id_; }
    auto Incoming(
        const opentxs::PaymentCode& sender,
        const Bip32Index index,
        const blockchain::Type chain,
        const PasswordPrompt& reason,
        const std::uint8_t version) const noexcept -> ECKey final;
    auto Key() const noexcept -> HDKey final;
    auto Locator(const AllocateOutput destination, const std::uint8_t version)
        const noexcept -> bool final;
    auto Outgoing(
        const opentxs::PaymentCode& recipient,
        const Bip32Index index,
        const blockchain::Type chain,
        const PasswordPrompt& reason,
        const std::uint8_t version) const noexcept -> ECKey final;
    auto Serialize(AllocateOutput destination) const noexcept -> bool final;
    auto Serialize(Serialized& serialized) const noexcept -> bool final;
    auto Sign(
        const identity::credential::Base& credential,
        proto::Signature& sig,
        const PasswordPrompt& reason) const noexcept -> bool final;
    auto Sign(const Data& data, Data& output, const PasswordPrompt& reason)
        const noexcept -> bool final;
    auto Unblind(
        const ReadView blinded,
        const crypto::key::EllipticCurve& publicKey,
        const ReadView outpoint,
        const PasswordPrompt& reason) const noexcept
        -> std::unique_ptr<opentxs::PaymentCode> final;
    auto UnblindV3(
        const std::uint8_t version,
        const ReadView blinded,
        const crypto::key::EllipticCurve& publicKey,
        const PasswordPrompt& reason) const noexcept
        -> std::unique_ptr<opentxs::PaymentCode> final;
    auto Valid() const noexcept -> bool final;
    auto Verify(
        const proto::Credential& master,
        const proto::Signature& sourceSignature) const noexcept -> bool final;
    auto Version() const noexcept -> VersionNumber final { return version_; }

    auto AddPrivateKeys(
        std::string& seed,
        const Bip32Index index,
        const PasswordPrompt& reason) noexcept -> bool final;

    PaymentCode(
        const api::internal::Core& api,
        const std::uint8_t version,
        const bool hasBitmessage,
        const ReadView pubkey,
        const ReadView chaincode,
        const std::uint8_t bitmessageVersion,
        const std::uint8_t bitmessageStream
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
        ,
        std::unique_ptr<crypto::key::Secp256k1> key
#endif
        ) noexcept;

    ~PaymentCode() final = default;

private:
    friend opentxs::Factory;
    using VersionType = std::uint8_t;
    using Mask = std::array<std::byte, 64>;

    const api::internal::Core& api_;
    const VersionType version_;
    const bool hasBitmessage_;
    const OTData pubkey_;
    const OTSecret chain_code_;
    const std::uint8_t bitmessage_version_;
    const std::uint8_t bitmessage_stream_;
    const OTNymID id_;
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    std::shared_ptr<crypto::key::Secp256k1> key_;
#else
    OTAsymmetricKey key_;
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1

    static auto calculate_id(
        const api::internal::Core& api,
        const ReadView pubkey,
        const ReadView chaincode) noexcept -> OTNymID;
    static auto effective_version(
        VersionType requested,
        VersionType actual) noexcept(false) -> VersionType;

    auto apply_mask(const Mask& mask, BinaryPreimage& data) const noexcept
        -> void;
    auto apply_mask(const Mask& mask, BinaryPreimage_3& data) const noexcept
        -> void;
    auto base58_preimage() const noexcept -> Base58Preimage;
    auto base58_preimage_v3() const noexcept -> Base58Preimage_3;
    auto binary_preimage() const noexcept -> BinaryPreimage;
    auto binary_preimage_v3() const noexcept -> BinaryPreimage_3;
    auto calculate_mask_v1(
        const crypto::key::EllipticCurve& local,
        const crypto::key::EllipticCurve& remote,
        const ReadView outpoint,
        const PasswordPrompt& reason) const noexcept(false) -> Mask;
    auto calculate_mask_v3(
        const crypto::key::EllipticCurve& local,
        const crypto::key::EllipticCurve& remote,
        const ReadView pubkey,
        const PasswordPrompt& reason) const noexcept(false) -> Mask;
    auto clone() const noexcept -> PaymentCode* final
    {
        return new PaymentCode(*this);
    }
    auto derive_keys(
        const opentxs::PaymentCode& other,
        const Bip32Index local,
        const Bip32Index remote,
        const PasswordPrompt& reason) const noexcept(false)
        -> std::pair<ECKey, ECKey>;
    auto effective_version(VersionType in) const noexcept(false) -> VersionType
    {
        return effective_version(in, version_);
    }
    auto generate_elements_v1(
        const opentxs::PaymentCode& recipient,
        const Space& blind,
        Elements& output) const noexcept(false) -> void;
    auto generate_elements_v3(
        const opentxs::PaymentCode& recipient,
        const Space& blind,
        Elements& output) const noexcept(false) -> void;
    auto match_locator(const std::uint8_t version, const Space& element) const
        noexcept(false) -> bool;
    auto postprocess(const Secret& in) const noexcept(false) -> OTSecret;
    auto shared_secret_mask_v1(
        const crypto::key::EllipticCurve& local,
        const crypto::key::EllipticCurve& remote,
        const PasswordPrompt& reason) const noexcept(false) -> OTSecret;
    auto shared_secret_payment_v1(
        const crypto::key::EllipticCurve& local,
        const crypto::key::EllipticCurve& remote,
        const PasswordPrompt& reason) const noexcept(false) -> OTSecret;
    auto shared_secret_payment_v3(
        const crypto::key::EllipticCurve& local,
        const crypto::key::EllipticCurve& remote,
        const blockchain::Type chain,
        const PasswordPrompt& reason) const noexcept(false) -> OTSecret;
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    auto unblind_v1(
        const ReadView in,
        const Mask& mask,
        const crypto::EcdsaProvider& ecdsa,
        const PasswordPrompt& reason) const
        -> std::unique_ptr<opentxs::PaymentCode>;
    auto unblind_v3(
        const std::uint8_t version,
        const ReadView in,
        const Mask& mask,
        const crypto::EcdsaProvider& ecdsa,
        const PasswordPrompt& reason) const
        -> std::unique_ptr<opentxs::PaymentCode>;
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1

    PaymentCode() = delete;
    PaymentCode(const PaymentCode&);
    PaymentCode(PaymentCode&&) = delete;
    auto operator=(const PaymentCode&) -> PaymentCode&;
    auto operator=(PaymentCode&&) -> PaymentCode&;
};
}  // namespace opentxs::implementation
