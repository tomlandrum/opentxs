// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                 // IWYU pragma: associated
#include "1_Internal.hpp"               // IWYU pragma: associated
#include "core/crypto/PaymentCode.hpp"  // IWYU pragma: associated

#include <boost/endian/buffers.hpp>
#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "2_Factory.hpp"
#include "internal/api/Api.hpp"
#include "internal/blockchain/Params.hpp"
#include "internal/crypto/key/Factory.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.tpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/HDSeed.hpp"
#include "opentxs/api/crypto/Asymmetric.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Encode.hpp"
#include "opentxs/api/crypto/Hash.hpp"
#include "opentxs/api/crypto/Util.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/SignatureRole.hpp"
#include "opentxs/crypto/SecretStyle.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"
#include "opentxs/crypto/key/EllipticCurve.hpp"  // IWYU pragma: keep
#include "opentxs/crypto/key/HD.hpp"             // IWYU pragma: keep
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
#include "opentxs/crypto/key/Secp256k1.hpp"
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
#include "opentxs/crypto/library/AsymmetricProvider.hpp"
#include "opentxs/identity/credential/Base.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/Credential.pb.h"
#include "opentxs/protobuf/HDPath.pb.h"
#include "opentxs/protobuf/MasterCredentialParameters.pb.h"
#include "opentxs/protobuf/NymIDSource.pb.h"
#include "opentxs/protobuf/PaymentCode.pb.h"
#include "opentxs/protobuf/Signature.pb.h"
#include "opentxs/protobuf/verify/Credential.hpp"
#include "opentxs/protobuf/verify/PaymentCode.hpp"

template class opentxs::Pimpl<opentxs::PaymentCode>;

#define OT_METHOD "opentxs::implementation::PaymentCode::"

namespace be = boost::endian;

namespace opentxs
{
const VersionNumber PaymentCode::DefaultVersion{3};

using ReturnType = implementation::PaymentCode;

auto Factory::PaymentCode(
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
    ) noexcept -> std::unique_ptr<opentxs::PaymentCode>
{
    return std::make_unique<ReturnType>(
        api,
        version,
        hasBitmessage,
        pubkey,
        chaincode,
        bitmessageVersion,
        bitmessageStream
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
        ,
        std::move(key)
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    );
}
}  // namespace opentxs

namespace opentxs::implementation
{
const std::size_t PaymentCode::pubkey_size_{sizeof(XpubPreimage::key_)};
const std::size_t PaymentCode::chain_code_size_{sizeof(XpubPreimage::code_)};

PaymentCode::PaymentCode(
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
    ) noexcept
    : api_(api)
    , version_(version)
    , hasBitmessage_(hasBitmessage)
    , pubkey_(api.Factory().Data(pubkey))
    , chain_code_(api.Factory().SecretFromBytes(chaincode))
    , bitmessage_version_(bitmessageVersion)
    , bitmessage_stream_(bitmessageStream)
    , id_(calculate_id(api, pubkey, chaincode))
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    , key_(std::move(key))
#else
    , key_(crypto::key::Asymmetric::Factory())
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
{
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    OT_ASSERT(key_);
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
}

PaymentCode::PaymentCode(const PaymentCode& rhs)
    : api_(rhs.api_)
    , version_(rhs.version_)
    , hasBitmessage_(rhs.hasBitmessage_)
    , pubkey_(rhs.pubkey_)
    , chain_code_(rhs.chain_code_)
    , bitmessage_version_(rhs.bitmessage_version_)
    , bitmessage_stream_(rhs.bitmessage_stream_)
    , id_(rhs.id_)
    , key_(rhs.key_)
{
}

PaymentCode::operator const crypto::key::Asymmetric&() const noexcept
{
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    return *key_;
#else
    return key_.get();
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
}

auto PaymentCode::operator==(const proto::PaymentCode& rhs) const noexcept
    -> bool
{
    const auto LHData = api_.Factory().Data(Serialize());
    const auto RHData = api_.Factory().Data(rhs);

    return (LHData == RHData);
}

auto PaymentCode::AddPrivateKeys(
    std::string& seed,
    const Bip32Index index,
    const PasswordPrompt& reason) noexcept -> bool
{
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
    auto pCandidate =
        api_.Seeds().GetPaymentCode(seed, index, version_, reason);

    if (false == bool(pCandidate)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to derive private key")
            .Flush();

        return false;
    }

    const auto& candidate = *pCandidate;

    if (0 != pubkey_->Bytes().compare(candidate.PublicKey())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Derived public key does not match this payment code")
            .Flush();

        return false;
    }

    if (0 != chain_code_->Bytes().compare(candidate.Chaincode(reason))) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Derived chain code does not match this payment code")
            .Flush();

        return false;
    }

    key_ = std::move(pCandidate);

    OT_ASSERT(key_);

    return true;
#else

    return false;
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
}

auto PaymentCode::apply_mask(const Mask& mask, BinaryPreimage& pre)
    const noexcept -> void
{
    static_assert(80 == sizeof(pre));

    auto i = reinterpret_cast<std::byte*>(&pre);
    std::advance(i, 3);

    for (const auto& m : mask) {
        auto& byte = *i;
        byte ^= m;
        std::advance(i, 1);
    }
}

auto PaymentCode::apply_mask(const Mask& mask, BinaryPreimage_3& pre)
    const noexcept -> void
{
    static_assert(34 == sizeof(pre));

    auto i = std::next(pre.key_.begin());
    const auto end = pre.key_.end();

    for (const auto& m : mask) {
        auto& byte = *i;
        byte ^= m;
        std::advance(i, 1);

        if (i == end) { break; }
    }
}

auto PaymentCode::asBase58() const noexcept -> std::string
{
    switch (version_) {
        case 1:
        case 2: {
            return api_.Crypto().Encode().IdentifierEncode(
                api_.Factory().Data(base58_preimage()));
        }
        case 3:
        default: {
            return api_.Crypto().Encode().IdentifierEncode(
                api_.Factory().Data(base58_preimage_v3()));
        }
    }
}

auto PaymentCode::base58_preimage() const noexcept -> Base58Preimage
{
    return Base58Preimage{binary_preimage()};
}

auto PaymentCode::base58_preimage_v3() const noexcept -> Base58Preimage_3
{
    return Base58Preimage_3{binary_preimage_v3()};
}

auto PaymentCode::binary_preimage() const noexcept -> BinaryPreimage
{
    return BinaryPreimage{
        version_,
        hasBitmessage_,
        pubkey_->Bytes(),
        chain_code_->Bytes(),
        bitmessage_version_,
        bitmessage_stream_};
}

auto PaymentCode::binary_preimage_v3() const noexcept -> BinaryPreimage_3
{
    return BinaryPreimage_3{version_, pubkey_->Bytes()};
}

auto PaymentCode::Blind(
    const opentxs::PaymentCode& recipient,
    const crypto::key::EllipticCurve& privateKey,
    const ReadView outpoint,
    const AllocateOutput dest,
    const PasswordPrompt& reason) const noexcept -> bool
{
    try {
        if (2 < recipient.Version()) {
            throw std::runtime_error{"Recipient payment code version too high"};
        }

        const auto pHD = recipient.Key();

        if (!pHD) {
            throw std::runtime_error{"Failed to obtain remote hd key"};
        }

        const auto& hd = *pHD;
        const auto pRemotePublic = hd.ChildKey(0, reason);

        if (!pRemotePublic) {
            throw std::runtime_error{
                "Failed to derive remote notification key"};
        }

        const auto& remotePublic = *pRemotePublic;
        const auto mask =
            calculate_mask_v1(privateKey, remotePublic, outpoint, reason);
        auto pre = binary_preimage();
        apply_mask(mask, pre);

        if (!dest) { throw std::runtime_error{"Invalid output allocator"}; }

        const auto view = pre.operator ReadView();
        const auto size = view.size();
        auto out = dest(size);

        if (false == out.valid(size)) {
            throw std::runtime_error{"Failed to allocate output space"};
        }

        std::memcpy(out.data(), view.data(), size);
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return false;
    }

    return true;
}

auto PaymentCode::BlindV3(
    const opentxs::PaymentCode& recipient,
    const crypto::key::EllipticCurve& privateKey,
    const AllocateOutput dest,
    const PasswordPrompt& reason) const noexcept -> bool
{
    try {
        if (3 > recipient.Version()) {
            throw std::runtime_error{"Recipient payment code version too low"};
        }

        const auto pHD = recipient.Key();

        if (!pHD) {
            throw std::runtime_error{"Failed to obtain remote hd key"};
        }

        const auto& hd = *pHD;
        const auto pRemotePublic = hd.ChildKey(0, reason);

        if (!pRemotePublic) {
            throw std::runtime_error{
                "Failed to derive remote notification key"};
        }

        if (!dest) { throw std::runtime_error{"Invalid output allocator"}; }

        const auto& remotePublic = *pRemotePublic;
        const auto mask = calculate_mask_v3(
            privateKey, remotePublic, privateKey.PublicKey(), reason);
        const auto copy_preimage = [&](const ReadView view) {
            const auto size = view.size();
            auto out = dest(size);

            if (false == out.valid(size)) {
                throw std::runtime_error{"Failed to allocate output space"};
            }

            std::memcpy(out.data(), view.data(), size);
        };

        switch (version_) {
            case 1:
            case 2: {
                auto pre = binary_preimage();
                apply_mask(mask, pre);
                copy_preimage(pre);
            } break;
            case 3:
            default: {
                auto pre = binary_preimage_v3();
                apply_mask(mask, pre);
                copy_preimage(
                    {reinterpret_cast<const char*>(pre.key_.data()),
                     pre.key_.size()});
            }
        }
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return false;
    }

    return true;
}

auto PaymentCode::calculate_id(
    const api::internal::Core& api,
    const ReadView key,
    const ReadView code) noexcept -> OTNymID
{
    auto output = api.Factory().NymID();

    if ((nullptr == key.data()) || (nullptr == code.data())) { return output; }

    auto preimage = api.Factory().Data();
    const auto target{pubkey_size_ + chain_code_size_};
    auto raw = preimage->WriteInto()(target);

    OT_ASSERT(raw.valid(target));

    auto* it = raw.as<std::byte>();
    std::memcpy(
        it, key.data(), std::min(key.size(), std::size_t{pubkey_size_}));
    std::advance(it, pubkey_size_);
    std::memcpy(
        it, code.data(), std::min(code.size(), std::size_t{chain_code_size_}));

    output->CalculateDigest(preimage->Bytes());

    return output;
}

auto PaymentCode::calculate_mask_v1(
    const crypto::key::EllipticCurve& local,
    const crypto::key::EllipticCurve& remote,
    const ReadView outpoint,
    const PasswordPrompt& reason) const noexcept(false) -> Mask
{
    auto mask = Mask{};
    const auto secret = shared_secret_mask_v1(local, remote, reason);
    const auto hashed = api_.Crypto().Hash().HMAC(
        crypto::HashType::Sha512,
        outpoint,
        secret->Bytes(),
        preallocated(mask.size(), mask.data()));

    if (false == hashed) { throw std::runtime_error{"Failed to derive mask"}; }

    return mask;
}

auto PaymentCode::calculate_mask_v3(
    const crypto::key::EllipticCurve& local,
    const crypto::key::EllipticCurve& remote,
    const ReadView pubkey,
    const PasswordPrompt& reason) const noexcept(false) -> Mask
{
    auto mask = Mask{};
    const auto secret = shared_secret_mask_v1(local, remote, reason);
    const auto hashed = api_.Crypto().Hash().HMAC(
        crypto::HashType::Sha512,
        secret->Bytes(),
        pubkey,
        preallocated(mask.size(), mask.data()));

    if (false == hashed) { throw std::runtime_error{"Failed to derive mask"}; }

    return mask;
}

auto PaymentCode::DecodeNotificationElements(
    const std::uint8_t version,
    const Elements& in,
    const PasswordPrompt& reason) const noexcept
    -> std::unique_ptr<opentxs::PaymentCode>
{
    try {
        if (3 > version_) {
            throw std::runtime_error{"Payment code version too low"};
        }

        if (3 != in.size()) {
            throw std::runtime_error{"Wrong number of elements"};
        }

        const auto& A = in.at(0);
        const auto& F = in.at(1);
        const auto& G = in.at(2);

        if (33 != A.size()) {
            throw std::runtime_error{
                std::string{"Invalid A ("} + std::to_string(A.size()) +
                ") bytes"};
        }

        if (false == match_locator(version, F)) {
            throw std::runtime_error{"Invalid locator"};
        }

        const auto blind = [&] {
            if (2 < version) {
                if (33 != G.size()) {
                    throw std::runtime_error{
                        std::string{"Invalid G ("} + std::to_string(G.size()) +
                        ") bytes"};
                }

                return G;
            } else {
                if (65 != F.size()) {
                    throw std::runtime_error{
                        std::string{"Invalid F ("} + std::to_string(F.size()) +
                        ") bytes"};
                }
                if (65 != G.size()) {
                    throw std::runtime_error{
                        std::string{"Invalid G ("} + std::to_string(G.size()) +
                        ") bytes"};
                }

                auto out = space(sizeof(BinaryPreimage));
                auto* o = out.data();
                {
                    auto* f = F.data();
                    std::advance(f, 33);
                    std::memcpy(o, f, 32);
                    std::advance(o, 32);
                }
                {
                    auto* g = G.data();
                    std::advance(g, 1);
                    std::memcpy(o, g, 48);
                }

                return out;
            }
        }();

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
        const auto pKey =
            api_.Asymmetric().InstantiateSecp256k1Key(reader(A), reason);

        if (!pKey) {
            throw std::runtime_error{"Failed to instantiate public key"};
        }

        const auto& key = *pKey;

        return UnblindV3(version, reader(blind), key, reason);
#else
        throw std::runtime_error{"Missing sepc256k1 support"};
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
    } catch (const std::exception& e) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto PaymentCode::derive_keys(
    const opentxs::PaymentCode& other,
    const Bip32Index local,
    const Bip32Index remote,
    const PasswordPrompt& reason) const noexcept(false)
    -> std::pair<ECKey, ECKey>
{
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    auto output = std::pair<ECKey, ECKey>{};
    auto& [localPrivate, remotePublic] = output;

    if (key_) {
        localPrivate = key_->ChildKey(local, reason);

        if (!localPrivate) {
            throw std::runtime_error("Failed to derive local private key");
        }
    } else {
        throw std::runtime_error("Failed to obtain local hd key");
    }

    if (const auto pKey = other.Key(); pKey) {
        const auto& key = *pKey;

        OT_ASSERT(0 < key.Chaincode(reason).size());

        remotePublic = key.ChildKey(remote, reason);

        if (!remotePublic) {
            throw std::runtime_error("Failed to derive remote public key");
        }
    } else {
        throw std::runtime_error("Failed to obtain remote hd key");
    }

    return output;
#else
    throw std::runtime_error("Missing secp256k1 support");
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
}

auto PaymentCode::effective_version(
    VersionType requested,
    VersionType actual) noexcept(false) -> VersionType
{
    auto version = (0 == requested) ? actual : requested;

    if (version > actual) {
        const auto error =
            std::string{"Requested version ("} + std::to_string(version) +
            ") is higher than allowed (" + std::to_string(actual) + ")";

        throw std::runtime_error(error);
    }

    return version;
}

auto PaymentCode::GenerateNotificationElements(
    const opentxs::PaymentCode& recipient,
    const crypto::key::EllipticCurve& privateKey,
    const PasswordPrompt& reason) const noexcept -> Elements
{
    try {
        if (3 > recipient.Version()) {
            throw std::runtime_error{"Recipient payment code version too low"};
        }

        const auto blind = [&] {
            auto out = Space{};
            const auto rc = BlindV3(recipient, privateKey, writer(out), reason);

            if (false == rc) {
                throw std::runtime_error{"Failed to blind payment code"};
            }

            return out;
        }();

        auto output = Elements{};
        constexpr auto size = sizeof(BinaryPreimage_3::key_);
        {
            auto& A = output.emplace_back(space(size));
            const auto rc =
                copy(privateKey.PublicKey(), preallocated(A.size(), A.data()));

            if (false == rc) {
                throw std::runtime_error{"Failed to copy public key"};
            }
        }

        switch (version_) {
            case 1:
            case 2: {
                generate_elements_v1(recipient, blind, output);
            } break;
            case 3:
            default: {
                generate_elements_v3(recipient, blind, output);
            }
        }

        return output;
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto PaymentCode::generate_elements_v1(
    const opentxs::PaymentCode& recipient,
    const Space& blind,
    Elements& output) const noexcept(false) -> void
{
    constexpr auto size = std::size_t{65};

    OT_ASSERT(blind.size() == sizeof(BinaryPreimage));

    auto* b = blind.data();
    {
        auto& F = output.emplace_back(space(size));
        auto* i = F.data();
        *i = std::byte{0x04};
        std::advance(i, 1);
        const auto rc = recipient.Locator(preallocated(32, i), version_);
        std::advance(i, 32);

        if (false == rc) { throw std::runtime_error{"Failed to copy locator"}; }
        std::memcpy(i, b, 32);
        std::advance(b, 32);
    }
    {
        auto& G = output.emplace_back(space(size));
        auto* i = G.data();
        *i = std::byte{0x04};
        std::advance(i, 1);
        std::memcpy(i, b, 48);
        std::advance(i, 48);
        api_.Crypto().Util().RandomizeMemory(i, 16);
    }
}

auto PaymentCode::generate_elements_v3(
    const opentxs::PaymentCode& recipient,
    const Space& blind,
    Elements& output) const noexcept(false) -> void
{
    constexpr auto size = sizeof(BinaryPreimage_3::key_);

    OT_ASSERT(blind.size() == size);

    {
        auto& F = output.emplace_back(space(size));
        auto* i = F.data();
        *i = std::byte{0x02};
        std::advance(i, 1);
        const auto rc = recipient.Locator(preallocated(size - 1, i), version_);

        if (false == rc) { throw std::runtime_error{"Failed to copy locator"}; }
    }
    {
        // G
        output.emplace_back(blind.begin(), blind.end());
    }
}

auto PaymentCode::Incoming(
    const opentxs::PaymentCode& sender,
    const Bip32Index index,
    const blockchain::Type chain,
    const PasswordPrompt& reason,
    const std::uint8_t version) const noexcept -> ECKey
{
    try {
        const auto effective = effective_version(version);
        const auto [pPrivate, pPublic] = derive_keys(sender, index, 0, reason);
        const auto& localPrivate = *pPrivate;
        const auto& remotePublic = *pPublic;

        switch (effective) {
            case 1:
            case 2: {
                const auto secret = shared_secret_payment_v1(
                    localPrivate, remotePublic, reason);

                return localPrivate.IncrementPrivate(secret, reason);
            }
            case 3: {
                const auto secret = shared_secret_payment_v3(
                    localPrivate, remotePublic, chain, reason);

                return localPrivate.IncrementPrivate(secret, reason);
            }
            default: {
                const auto error = std::string{"Unsupported version "} +
                                   std::to_string(effective);

                throw std::runtime_error{error};
            }
        }
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto PaymentCode::Key() const noexcept -> HDKey
{
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1

    return key_;
#else

    return {};
#endif
}

auto PaymentCode::Locator(const AllocateOutput dest, const std::uint8_t version)
    const noexcept -> bool
{
    try {
        switch (version_) {
            case 1: {
                const auto error =
                    std::string{"Locator not defined for version "} +
                    std::to_string(version_);

                throw std::runtime_error(error);
            }
            case 2: {
                const auto pre = [&] {
                    auto out = std::array<std::byte, 33>{};
                    out[0] = std::byte{0x02};
                    const auto hashed = api_.Crypto().Hash().Digest(
                        crypto::HashType::Sha256,
                        binary_preimage(),
                        preallocated(out.size() - 1, std::next(out.data(), 1)));

                    if (false == hashed) {
                        throw std::runtime_error(
                            "Failed to calculate version 2 hash");
                    }

                    return out;
                }();
                constexpr auto size = pre.size();

                if (false == bool(dest)) {
                    throw std::runtime_error("Invalid output allocator");
                }

                auto out = dest(size);

                if (false == out.valid(size)) {
                    throw std::runtime_error("Failed to allocate output space");
                }

                std::memcpy(out.data(), pre.data(), pre.size());
            } break;
            case 3:
            default: {
                const auto effective = (0 == version) ? version_ : version;
                auto hash = space(64);
                auto rc = api_.Crypto().Hash().HMAC(
                    crypto::HashType::Sha512,
                    chain_code_->Bytes(),
                    ReadView{
                        reinterpret_cast<const char*>(&effective),
                        sizeof(effective)},
                    writer(hash));

                if (false == rc) {
                    throw std::runtime_error("Failed to hash locator");
                }

                if (false == copy(reader(hash), dest, 32)) {
                    throw std::runtime_error("Failed to copy locator");
                }
            }
        }
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return false;
    }

    return true;
}

auto PaymentCode::match_locator(
    const std::uint8_t version,
    const Space& element) const noexcept(false) -> bool
{
    if (sizeof(BinaryPreimage_3::key_) > element.size()) {
        throw std::runtime_error{"Invalid F"};
    }

    const auto id = [&] {
        auto out = Space{};

        if (false == Locator(writer(out), version)) {
            throw std::runtime_error{"Failed to calculate locator"};
        }

        return out;
    }();

    OT_ASSERT(id.size() < element.size());

    return 0 == std::memcmp(std::next(element.data()), id.data(), id.size());
}

auto PaymentCode::Outgoing(
    const opentxs::PaymentCode& recipient,
    const Bip32Index index,
    const blockchain::Type chain,
    const PasswordPrompt& reason,
    const std::uint8_t version) const noexcept -> ECKey
{
    try {
        if (false == key_->HasPrivate()) {
            throw std::runtime_error{"Private key missing"};
        }

        const auto effective = effective_version(version, recipient.Version());
        const auto [pPrivate, pPublic] =
            derive_keys(recipient, 0, index, reason);
        const auto& localPrivate = *pPrivate;
        const auto& remotePublic = *pPublic;

        switch (effective) {
            case 1:
            case 2: {
                const auto secret = shared_secret_payment_v1(
                    localPrivate, remotePublic, reason);

                return remotePublic.IncrementPublic(secret);
            }
            case 3: {
                const auto secret = shared_secret_payment_v3(
                    localPrivate, remotePublic, chain, reason);

                return remotePublic.IncrementPublic(secret);
            }
            default: {
                const auto error = std::string{"Unsupported version "} +
                                   std::to_string(effective);

                throw std::runtime_error{error};
            }
        }
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto PaymentCode::postprocess(const Secret& in) const noexcept(false)
    -> OTSecret
{
    auto output = api_.Factory().Secret({});
    auto rc = api_.Crypto().Hash().Digest(
        crypto::HashType::Sha256, in.Bytes(), output->WriteInto());

    if (false == rc) {
        throw std::runtime_error{"Failed to hash shared secret"};
    }

    return output;
}

auto PaymentCode::Serialize() const noexcept -> Serialized
{
    const auto key = pubkey_->Bytes();
    const auto code = chain_code_->Bytes();
    auto output = Serialized{};
    output.set_version(version_);
    output.set_key(key.data(), key.size());
    output.set_chaincode(code.data(), code.size());
    output.set_bitmessageversion(bitmessage_version_);
    output.set_bitmessagestream(bitmessage_stream_);

    return output;
}

auto PaymentCode::shared_secret_mask_v1(
    const crypto::key::EllipticCurve& local,
    const crypto::key::EllipticCurve& remote,
    const PasswordPrompt& reason) const noexcept(false) -> OTSecret
{
    auto output = api_.Factory().Secret(0);
    auto rc = local.engine().SharedSecret(
        remote, local, crypto::SecretStyle::X_only, reason, output);

    if (false == rc) {
        throw std::runtime_error{"Failed to calculate shared secret"};
    }

    return output;
}

auto PaymentCode::shared_secret_payment_v1(
    const crypto::key::EllipticCurve& local,
    const crypto::key::EllipticCurve& remote,
    const PasswordPrompt& reason) const noexcept(false) -> OTSecret
{
    auto secret = shared_secret_mask_v1(local, remote, reason);

    return postprocess(secret);
}

auto PaymentCode::shared_secret_payment_v3(
    const crypto::key::EllipticCurve& local,
    const crypto::key::EllipticCurve& remote,
    const blockchain::Type chain,
    const PasswordPrompt& reason) const noexcept(false) -> OTSecret
{
    auto secret = shared_secret_mask_v1(local, remote, reason);
    auto hmac = api_.Factory().Secret({});
    const auto bip44 = be::big_uint32_buf_t{static_cast<std::uint32_t>(
        blockchain::params::Data::Chains().at(chain).bip44_)};

    static_assert(sizeof(bip44) == sizeof(std::uint32_t));

    auto rc = api_.Crypto().Hash().HMAC(
        crypto::HashType::Sha512,
        secret->Bytes(),
        ReadView{reinterpret_cast<const char*>(&bip44), sizeof(bip44)},
        hmac->WriteInto());

    if (false == rc) {
        throw std::runtime_error{"Failed to calculate shared secret hmac"};
    }

    return postprocess(hmac);
}

auto PaymentCode::Sign(
    const identity::credential::Base& credential,
    proto::Signature& sig,
    const PasswordPrompt& reason) const noexcept -> bool
{
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    auto serialized = credential.Serialized(AS_PUBLIC, WITHOUT_SIGNATURES);
    auto& signature = *serialized->add_signature();
    const bool output = key_->Sign(
        [&]() -> std::string { return proto::ToString(*serialized); },
        crypto::SignatureRole::NymIDSource,
        signature,
        ID(),
        reason);
    sig.CopyFrom(signature);

    return output;
#else

    return false;
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
}

auto PaymentCode::Sign(
    const Data& data,
    Data& output,
    const PasswordPrompt& reason) const noexcept -> bool
{
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    const auto& key = *key_;

    return key.engine().Sign(
        api_,
        data.Bytes(),
        key,
        crypto::HashType::Sha256,
        output.WriteInto(),
        reason);
#else

    return false;
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
}

auto PaymentCode::Unblind(
    const ReadView in,
    const crypto::key::EllipticCurve& remote,
    const ReadView outpoint,
    const PasswordPrompt& reason) const noexcept
    -> std::unique_ptr<opentxs::PaymentCode>
{
    try {
        if (2 < version_) {
            throw std::runtime_error{"Payment code version too high"};
        }

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
        if (!key_) { throw std::runtime_error{"Missing private key"}; }

        const auto pLocal = key_->ChildKey(0, reason);

        if (!pLocal) {
            throw std::runtime_error{"Failed to derive notification key"};
        }

        const auto& local = *pLocal;
        const auto mask = calculate_mask_v1(local, remote, outpoint, reason);
        auto pre = [&] {
            auto out = BinaryPreimage{};

            if ((nullptr == in.data()) || (in.size() != sizeof(out))) {
                throw std::runtime_error{"Invalid blinded payment code"};
            }

            std::memcpy(reinterpret_cast<void*>(&out), in.data(), in.size());

            return out;
        }();
        apply_mask(mask, pre);

        return std::make_unique<PaymentCode>(
            api_,
            pre.version_,
            pre.haveBitmessage(),
            pre.xpub_.Key(),
            pre.xpub_.Chaincode(),
            pre.bm_version_,
            pre.bm_stream_,
            factory::Secp256k1Key(
                api_,
                local.ECDSA(),
                api_.Factory().Secret(0),
                api_.Factory().SecretFromBytes(pre.xpub_.Chaincode()),
                api_.Factory().Data(pre.xpub_.Key()),
                proto::HDPath{},
                Bip32Fingerprint{},
                proto::KEYROLE_SIGN,
                crypto::key::EllipticCurve::DefaultVersion,
                reason));
#else
        throw std::runtime_error{"Missing secp256k1 support"};
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto PaymentCode::UnblindV3(
    const std::uint8_t version,
    const ReadView in,
    const crypto::key::EllipticCurve& remote,
    const PasswordPrompt& reason) const noexcept
    -> std::unique_ptr<opentxs::PaymentCode>
{
    try {
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
        if (3 > version_) {
            throw std::runtime_error{"Local payment code version too low"};
        }

        if (!key_) { throw std::runtime_error{"Missing private key"}; }

        const auto pLocal = key_->ChildKey(0, reason);

        if (!pLocal) {
            throw std::runtime_error{"Failed to derive notification key"};
        }

        const auto& local = *pLocal;
        const auto mask =
            calculate_mask_v3(local, remote, remote.PublicKey(), reason);

        switch (version) {
            case 1:
            case 2: {
                return unblind_v1(in, mask, local.ECDSA(), reason);
            }
            case 3:
            default: {
                return unblind_v3(version, in, mask, local.ECDSA(), reason);
            }
        }
#else
        throw std::runtime_error{"Missing secp256k1 support"};
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
auto PaymentCode::unblind_v1(
    const ReadView in,
    const Mask& mask,
    const crypto::EcdsaProvider& ecdsa,
    const PasswordPrompt& reason) const -> std::unique_ptr<opentxs::PaymentCode>
{
    const auto pre = [&] {
        auto out = BinaryPreimage{};

        if ((nullptr == in.data()) || (in.size() != (sizeof(out)))) {
            throw std::runtime_error{"Invalid blinded payment code (v1)"};
        }

        auto i = reinterpret_cast<char*>(reinterpret_cast<void*>(&out));
        std::memcpy(i, in.data(), in.size());
        apply_mask(mask, out);

        return out;
    }();

    return std::make_unique<PaymentCode>(
        api_,
        pre.version_,
        pre.haveBitmessage(),
        pre.xpub_.Key(),
        pre.xpub_.Chaincode(),
        pre.bm_version_,
        pre.bm_stream_,
        factory::Secp256k1Key(
            api_,
            ecdsa,
            api_.Factory().Secret(0),
            api_.Factory().SecretFromBytes(pre.xpub_.Chaincode()),
            api_.Factory().Data(pre.xpub_.Key()),
            proto::HDPath{},
            Bip32Fingerprint{},
            proto::KEYROLE_SIGN,
            crypto::key::EllipticCurve::DefaultVersion,
            reason));
}

auto PaymentCode::unblind_v3(
    const std::uint8_t version,
    const ReadView in,
    const Mask& mask,
    const crypto::EcdsaProvider& ecdsa,
    const PasswordPrompt& reason) const -> std::unique_ptr<opentxs::PaymentCode>
{
    const auto pre = [&] {
        auto out = BinaryPreimage_3{};
        out.version_ = version;

        if ((nullptr == in.data()) || ((in.size() + 1u) != sizeof(out))) {
            throw std::runtime_error{"Invalid blinded payment code (v3)"};
        }

        auto i = reinterpret_cast<char*>(reinterpret_cast<void*>(&out));
        std::memcpy(std::next(i), in.data(), in.size());
        apply_mask(mask, out);

        return out;
    }();
    const auto code = [&] {
        auto out = api_.Factory().Secret(0);
        const auto rc = api_.Crypto().Hash().Digest(
            crypto::HashType::Sha256D, pre.Key(), out->WriteInto());

        if (false == rc) {
            throw std::runtime_error{"Failed to calculate chain code"};
        }

        return out;
    }();

    return std::make_unique<PaymentCode>(
        api_,
        pre.version_,
        false,
        pre.Key(),
        code->Bytes(),
        0,
        0,
        factory::Secp256k1Key(
            api_,
            ecdsa,
            api_.Factory().Secret(0),
            code,
            api_.Factory().Data(pre.Key()),
            proto::HDPath{},
            Bip32Fingerprint{},
            proto::KEYROLE_SIGN,
            crypto::key::EllipticCurve::DefaultVersion,
            reason));
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1

auto PaymentCode::Valid() const noexcept -> bool
{
    if (0 == version_) { return false; }

    if (pubkey_size_ != pubkey_->size()) { return false; }

    if (chain_code_size_ != chain_code_->size()) { return false; }

    return proto::Validate<proto::PaymentCode>(Serialize(), SILENT);
}

auto PaymentCode::Verify(
    const proto::Credential& master,
    const proto::Signature& sourceSignature) const noexcept -> bool
{
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    if (false == proto::Validate<proto::Credential>(
                     master,
                     VERBOSE,
                     proto::KEYMODE_PUBLIC,
                     proto::CREDROLE_MASTERKEY,
                     false)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Invalid master credential syntax.")
            .Flush();

        return false;
    }

    const bool sameSource =
        (*this == master.masterdata().source().paymentcode());

    if (false == sameSource) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Master credential was not derived from this source.")
            .Flush();

        return false;
    }

    auto copy = proto::Credential{};
    copy.CopyFrom(master);
    auto& signature = *copy.add_signature();
    signature.CopyFrom(sourceSignature);
    signature.clear_signature();

    return key_->Verify(api_.Factory().Data(copy), sourceSignature);
#else

    return false;
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
}
}  // namespace opentxs::implementation
