// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_IDENTITY_CREDENTIAL_BASE_HPP
#define OPENTXS_IDENTITY_CREDENTIAL_BASE_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>
#include <string>

#include "opentxs/Types.hpp"
#include "opentxs/core/contract/Signable.hpp"
#include "opentxs/crypto/key/asymmetric/Role.hpp"
#include "opentxs/identity/Types.hpp"

namespace opentxs
{
namespace proto
{
class Credential;
class ContactData;
class VerificationSet;
}  // namespace proto

class PasswordPrompt;
class Secret;
}  // namespace opentxs

namespace opentxs
{
namespace identity
{
namespace credential
{
class OPENTXS_EXPORT Base : virtual public opentxs::contract::Signable
{
public:
    using SerializedType = proto::Credential;

    virtual std::string asString(const bool asPrivate = false) const = 0;
    virtual const Identifier& CredentialID() const = 0;
    virtual bool GetContactData(proto::ContactData& contactData) const = 0;
    virtual bool GetVerificationSet(
        proto::VerificationSet& verificationSet) const = 0;
    virtual bool hasCapability(const NymCapability& capability) const = 0;
    virtual Signature MasterSignature() const = 0;
    virtual crypto::key::asymmetric::Mode Mode() const = 0;
    virtual identity::CredentialRole Role() const = 0;
    virtual bool Private() const = 0;
    virtual bool Save() const = 0;
    virtual Signature SelfSignature(
        CredentialModeFlag version = PUBLIC_VERSION) const = 0;
    using Signable::Serialize;
    OPENTXS_NO_EXPORT virtual bool Serialize(
        SerializedType& serialized,
        const SerializationModeFlag asPrivate,
        const SerializationSignatureFlag asSigned) const = 0;
    virtual Signature SourceSignature() const = 0;
    virtual bool TransportKey(
        Data& publicKey,
        Secret& privateKey,
        const PasswordPrompt& reason) const = 0;
    virtual identity::CredentialType Type() const = 0;
    virtual bool Verify(
        const Data& plaintext,
        const proto::Signature& sig,
        const opentxs::crypto::key::asymmetric::Role key =
            opentxs::crypto::key::asymmetric::Role::Sign) const = 0;
    virtual bool Verify(
        const proto::Credential& credential,
        const identity::CredentialRole& role,
        const Identifier& masterID,
        const proto::Signature& masterSig) const = 0;

    ~Base() override = default;

protected:
    Base() noexcept {}  // TODO Signable

private:
    Base(const Base&) = delete;
    Base(Base&&) = delete;
    Base& operator=(const Base&) = delete;
    Base& operator=(Base&&) = delete;
};
}  // namespace credential
}  // namespace identity
}  // namespace opentxs
#endif
