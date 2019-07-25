// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

#include "crypto/library/AsymmetricProviderNull.hpp"

#include <memory>

namespace opentxs::crypto::key::implementation
{
class Null : virtual public key::Asymmetric
{
public:
    OTData CalculateHash(const proto::HashType, const PasswordPrompt&)
        const final
    {
        return Data::Factory();
    }
    bool CalculateID(Identifier&) const final { return false; }
    const opentxs::crypto::AsymmetricProvider& engine() const final { throw; }
    const OTSignatureMetadata* GetMetadata() const final { return nullptr; }
    bool hasCapability(const NymCapability&) const final { return false; }
    bool HasPrivate() const final { return false; }
    bool HasPublic() const final { return false; }
    proto::AsymmetricKeyType keyType() const final
    {
        return proto::AKEYTYPE_NULL;
    }
    bool Open(
        crypto::key::Asymmetric&,
        crypto::key::Symmetric&,
        PasswordPrompt&,
        const PasswordPrompt&) const final
    {
        return false;
    }
    const std::string Path() const final { return {}; }
    bool Path(proto::HDPath&) const final { return false; }
    const proto::KeyRole& Role() const final { throw; }
    std::shared_ptr<proto::AsymmetricKey> Serialize() const final
    {
        return nullptr;
    }
    OTData SerializeKeyToData(const proto::AsymmetricKey&) const final
    {
        return Data::Factory();
    }
    proto::HashType SigHashType() const final { return proto::HASHTYPE_NONE; }
    bool Sign(
        const Data&,
        proto::Signature&,
        const PasswordPrompt&,
        const OTPassword* = nullptr,
        const String& = String::Factory(""),
        const proto::SignatureRole = proto::SIGROLE_ERROR) const final
    {
        return false;
    }
    bool Sign(
        const GetPreimage,
        const proto::SignatureRole,
        proto::Signature&,
        const Identifier&,
        const PasswordPrompt&,
        proto::KeyRole,
        const proto::HashType) const final
    {
        return false;
    }
    bool TransportKey(Data&, OTPassword&, const PasswordPrompt& reason)
        const final
    {
        return false;
    }
    bool Verify(const Data&, const proto::Signature&, const PasswordPrompt&)
        const final
    {
        return false;
    }

    void Release() final {}
    void ReleaseKey() final {}
    bool Seal(
        const opentxs::api::Core&,
        OTAsymmetricKey&,
        crypto::key::Symmetric&,
        const PasswordPrompt&,
        PasswordPrompt&) const final
    {
        return false;
    }
    void SetAsPublic() final {}
    void SetAsPrivate() final {}

    operator bool() const final { return false; }
    bool operator==(const proto::AsymmetricKey&) const final { return false; }

    Null() = default;
    ~Null() = default;

private:
    Null* clone() const override { return new Null; }
};

#if OT_CRYPTO_SUPPORTED_KEY_HD
class NullEC : virtual public key::EllipticCurve, public Null
{
public:
    std::unique_ptr<EllipticCurve> asPublic(const PasswordPrompt&) const final
    {
        return {};
    }
    bool CheckCapability(const NymCapability&) const final { return {}; }
    const crypto::EcdsaProvider& ECDSA() const final { throw; }
    bool GetKey(Data&) const final { return {}; }
    bool GetKey(proto::Ciphertext&) const final { return {}; }
    const Asymmetric& GetPrivateKey() const final { throw; }
    const Asymmetric& GetPublicKey() const final { throw; }
    std::int32_t GetPublicKeyBySignature(Keys&, const Signature&, bool)
        const final
    {
        return {};
    }
    std::shared_ptr<proto::AsymmetricKey> GetSerialized(bool) const final
    {
        return {};
    }
    bool GetTransportKey(Data&, OTPassword&, const PasswordPrompt&) const final
    {
        return false;
    }
    OTData PrivateKey(const PasswordPrompt&) const final
    {
        return Data::Factory();
    }
    OTData PublicKey(const PasswordPrompt&) const final
    {
        return Data::Factory();
    }

    bool SetKey(const Data&) final { return {}; }
    bool SetKey(std::unique_ptr<proto::Ciphertext>&) final { return {}; }

    NullEC() = default;
    ~NullEC() = default;
};

class NullHD : virtual public key::HD, public NullEC
{
public:
    OTData Chaincode(const PasswordPrompt& reason) const final
    {
        return Data::Factory();
    }
    int Depth() const final { return {}; }
    Bip32Fingerprint Fingerprint(const PasswordPrompt& reason) const final
    {
        return {};
    }
    std::string Xprv(const PasswordPrompt& reason) const final { return {}; }
    std::string Xpub(const PasswordPrompt& reason) const final { return {}; }

    NullHD() = default;
    ~NullHD() = default;

private:
    NullHD* clone() const final { return new NullHD; }
};
#endif  // OT_CRYPTO_SUPPORTED_KEY_HD
}  // namespace opentxs::crypto::key::implementation
