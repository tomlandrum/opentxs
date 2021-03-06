// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CRYPTO_KEY_ASYMMETRIC_HPP
#define OPENTXS_CRYPTO_KEY_ASYMMETRIC_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <list>
#include <memory>
#include <string>

#include "opentxs/Bytes.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/crypto/HashType.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/crypto/library/AsymmetricProvider.hpp"

namespace opentxs
{
namespace crypto
{
namespace key
{
class Asymmetric;
}  // namespace key
}  // namespace crypto

namespace identity
{
class Authority;
}  // namespace identity

class OTSignatureMetadata;
class Secret;

using OTAsymmetricKey = Pimpl<crypto::key::Asymmetric>;
}  // namespace opentxs

namespace opentxs
{
namespace crypto
{
namespace key
{
class OPENTXS_EXPORT Asymmetric
{
public:
    using Serialized = proto::AsymmetricKey;

    static const VersionNumber DefaultVersion;
    static const VersionNumber MaxVersion;

    static OTAsymmetricKey Factory() noexcept;

    virtual std::unique_ptr<Asymmetric> asPublic() const noexcept = 0;
    virtual OTData CalculateHash(
        const crypto::HashType hashType,
        const PasswordPrompt& reason) const noexcept = 0;
    virtual bool CalculateID(Identifier& theOutput) const noexcept = 0;
    virtual bool CalculateTag(
        const identity::Authority& nym,
        const crypto::key::asymmetric::Algorithm type,
        const PasswordPrompt& reason,
        std::uint32_t& tag,
        Secret& password) const noexcept = 0;
    virtual bool CalculateTag(
        const Asymmetric& dhKey,
        const Identifier& credential,
        const PasswordPrompt& reason,
        std::uint32_t& tag) const noexcept = 0;
    virtual bool CalculateSessionPassword(
        const Asymmetric& dhKey,
        const PasswordPrompt& reason,
        Secret& password) const noexcept = 0;
    virtual const opentxs::crypto::AsymmetricProvider& engine()
        const noexcept = 0;
    virtual const OTSignatureMetadata* GetMetadata() const noexcept = 0;
    virtual bool hasCapability(
        const NymCapability& capability) const noexcept = 0;
    virtual bool HasPrivate() const noexcept = 0;
    virtual bool HasPublic() const noexcept = 0;
    virtual crypto::key::asymmetric::Algorithm keyType() const noexcept = 0;
    virtual ReadView Params() const noexcept = 0;
    virtual const std::string Path() const noexcept = 0;
    OPENTXS_NO_EXPORT virtual bool Path(
        proto::HDPath& output) const noexcept = 0;
    virtual ReadView PrivateKey(
        const PasswordPrompt& reason) const noexcept = 0;
    virtual ReadView PublicKey() const noexcept = 0;
    virtual opentxs::crypto::key::asymmetric::Role Role() const noexcept = 0;
    OPENTXS_NO_EXPORT virtual bool Serialize(
        Serialized& serialized) const noexcept = 0;
    virtual crypto::HashType SigHashType() const noexcept = 0;
    OPENTXS_NO_EXPORT virtual bool Sign(
        const GetPreimage input,
        const crypto::SignatureRole role,
        proto::Signature& signature,
        const Identifier& credential,
        const PasswordPrompt& reason,
        const crypto::HashType hash =
            crypto::HashType::Error) const noexcept = 0;
    virtual bool Sign(
        const ReadView preimage,
        const crypto::HashType hash,
        const AllocateOutput output,
        const PasswordPrompt& reason) const noexcept = 0;
    virtual bool TransportKey(
        Data& publicKey,
        Secret& privateKey,
        const PasswordPrompt& reason) const noexcept = 0;
    OPENTXS_NO_EXPORT virtual bool Verify(
        const Data& plaintext,
        const proto::Signature& sig) const noexcept = 0;
    virtual VersionNumber Version() const noexcept = 0;

    virtual operator bool() const noexcept = 0;
    OPENTXS_NO_EXPORT virtual bool operator==(
        const Serialized&) const noexcept = 0;

    virtual ~Asymmetric() = default;

protected:
    Asymmetric() = default;

private:
    friend OTAsymmetricKey;

    virtual Asymmetric* clone() const noexcept = 0;

    Asymmetric(const Asymmetric&) = delete;
    Asymmetric(Asymmetric&&) = delete;
    Asymmetric& operator=(const Asymmetric&) = delete;
    Asymmetric& operator=(Asymmetric&&) = delete;
};
}  // namespace key
}  // namespace crypto
}  // namespace opentxs
#endif
