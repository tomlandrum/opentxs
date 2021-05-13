// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>

#include "crypto/library/AsymmetricProviderNull.hpp"
#include "opentxs/crypto/key/HD.hpp"
#include "opentxs/crypto/key/Keypair.hpp"
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
#include "opentxs/crypto/key/Secp256k1.hpp"
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
#include "opentxs/crypto/key/asymmetric/Algorithm.hpp"
#include "opentxs/protobuf/AsymmetricKey.pb.h"

namespace opentxs::crypto::key::implementation
{
class NullKeypair final : virtual public key::Keypair
{
public:
    operator bool() const noexcept final { return false; }

    auto CheckCapability(const NymCapability&) const noexcept -> bool final
    {
        return {};
    }
    auto GetPrivateKey() const noexcept(false) -> const Asymmetric& final
    {
        throw std::runtime_error("");
    }
    auto GetPublicKey() const noexcept(false) -> const Asymmetric& final
    {
        throw std::runtime_error("");
    }
    auto GetPublicKeyBySignature(Keys&, const Signature&, bool) const noexcept
        -> std::int32_t final
    {
        return {};
    }
    auto Serialize(proto::AsymmetricKey& serialized, bool) const noexcept
        -> bool final
    {
        serialized = {};
        return {};
    }
    auto GetTransportKey(Data&, Secret&, const PasswordPrompt&) const noexcept
        -> bool final
    {
        return {};
    }

    auto clone() const -> NullKeypair* final { return new NullKeypair; }

    ~NullKeypair() final = default;
};

class Null : virtual public key::Asymmetric
{
public:
    auto asPublic() const noexcept -> std::unique_ptr<Asymmetric> final
    {
        return {};
    }
    auto CalculateHash(const crypto::HashType, const PasswordPrompt&)
        const noexcept -> OTData final
    {
        return Data::Factory();
    }
    auto CalculateID(Identifier&) const noexcept -> bool final { return false; }
    auto CalculateTag(
        const identity::Authority&,
        const crypto::key::asymmetric::Algorithm,
        const PasswordPrompt&,
        std::uint32_t&,
        Secret&) const noexcept -> bool final
    {
        return false;
    }
    auto CalculateTag(
        const Asymmetric&,
        const Identifier&,
        const PasswordPrompt&,
        std::uint32_t&) const noexcept -> bool final
    {
        return false;
    }
    auto CalculateSessionPassword(
        const Asymmetric&,
        const PasswordPrompt&,
        Secret&) const noexcept -> bool final
    {
        return false;
    }
    auto engine() const noexcept
        -> const opentxs::crypto::AsymmetricProvider& final
    {
        abort();
    }
    auto GetMetadata() const noexcept -> const OTSignatureMetadata* final
    {
        return nullptr;
    }
    auto hasCapability(const NymCapability&) const noexcept -> bool final
    {
        return false;
    }
    auto HasPrivate() const noexcept -> bool final { return false; }
    auto HasPublic() const noexcept -> bool final { return false; }
    auto keyType() const noexcept -> crypto::key::asymmetric::Algorithm final
    {
        return crypto::key::asymmetric::Algorithm::Null;
    }
    auto Params() const noexcept -> ReadView final { return {}; }
    auto Path() const noexcept -> const std::string final { return {}; }
    auto Path(proto::HDPath&) const noexcept -> bool final { return false; }
    auto PrivateKey(const PasswordPrompt&) const noexcept -> ReadView final
    {
        return {};
    }
    auto PublicKey() const noexcept -> ReadView final { return {}; }
    auto Role() const noexcept -> opentxs::crypto::key::asymmetric::Role final
    {
        return {};
    }
    auto Serialize(Serialized& serialized) const noexcept -> bool final
    {
        serialized = Serialized{};

        return true;
    }
    auto SigHashType() const noexcept -> crypto::HashType final
    {
        return crypto::HashType::None;
    }
    auto Sign(
        const GetPreimage,
        const crypto::SignatureRole,
        proto::Signature&,
        const Identifier&,
        const PasswordPrompt&,
        const crypto::HashType) const noexcept -> bool final
    {
        return false;
    }
    auto Sign(
        const ReadView,
        const crypto::HashType,
        const AllocateOutput,
        const PasswordPrompt&) const noexcept -> bool final
    {
        return false;
    }
    auto TransportKey(Data&, Secret&, const PasswordPrompt&) const noexcept
        -> bool final
    {
        return false;
    }
    auto Verify(const Data&, const proto::Signature&) const noexcept
        -> bool final
    {
        return false;
    }
    auto Version() const noexcept -> VersionNumber final { return {}; }

    operator bool() const noexcept override { return false; }
    auto operator==(const proto::AsymmetricKey&) const noexcept -> bool final
    {
        return false;
    }

    Null() = default;
    ~Null() override = default;

private:
    auto clone() const noexcept -> Null* override { return new Null; }
};

class NullEC : virtual public key::EllipticCurve, public Null
{
public:
    operator bool() const noexcept final { return false; }

    auto asPublicEC() const noexcept -> std::unique_ptr<EllipticCurve> final
    {
        return {};
    }
    auto CloneEC() const noexcept -> std::unique_ptr<EllipticCurve> final
    {
        return {};
    }
    auto ECDSA() const noexcept -> const opentxs::crypto::EcdsaProvider& final
    {
        abort();
    }
    auto IncrementPrivate(const Secret&, const PasswordPrompt&) const noexcept
        -> std::unique_ptr<key::EllipticCurve> final
    {
        return {};
    }
    auto IncrementPublic(const Secret&) const noexcept
        -> std::unique_ptr<key::EllipticCurve> final
    {
        return {};
    }
    auto SignDER(
        const ReadView,
        const crypto::HashType,
        Space&,
        const PasswordPrompt&) const noexcept -> bool final
    {
        return false;
    }

    NullEC() = default;
    ~NullEC() override = default;
};

class NullHD : virtual public key::HD, public NullEC
{
public:
    auto Chaincode(const PasswordPrompt&) const noexcept -> ReadView final
    {
        return {};
    }
    auto ChildKey(const Bip32Index, const PasswordPrompt&) const noexcept
        -> std::unique_ptr<key::HD> final
    {
        return {};
    }
    auto Depth() const noexcept -> int final { return {}; }
    auto Fingerprint() const noexcept -> Bip32Fingerprint final { return {}; }
    auto Parent() const noexcept -> Bip32Fingerprint final { return {}; }
    auto Xprv(const PasswordPrompt&) const noexcept -> std::string final
    {
        return {};
    }
    auto Xpub(const PasswordPrompt&) const noexcept -> std::string final
    {
        return {};
    }

    NullHD() = default;
    ~NullHD() override = default;

private:
    auto clone() const noexcept -> NullHD* override { return new NullHD; }
};

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
class NullSecp256k1 final : virtual public key::Secp256k1, public NullHD
{
public:
    NullSecp256k1() = default;
    ~NullSecp256k1() final = default;

private:
    auto clone() const noexcept -> NullSecp256k1* final
    {
        return new NullSecp256k1;
    }
};
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
}  // namespace opentxs::crypto::key::implementation
