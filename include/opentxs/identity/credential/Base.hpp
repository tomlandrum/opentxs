// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_IDENTITY_CREDENTIAL_BASE_HPP
#define OPENTXS_IDENTITY_CREDENTIAL_BASE_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>
#include <string>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/contract/Signable.hpp"
#include "opentxs/identity/Types.hpp"
#include "opentxs/protobuf/Enums.pb.h"

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
class Base : virtual public opentxs::contract::Signable
{
public:
    using SerializedType = proto::Credential;

    OPENTXS_EXPORT virtual std::string asString(
        const bool asPrivate = false) const = 0;
    OPENTXS_EXPORT virtual const Identifier& CredentialID() const = 0;
    OPENTXS_EXPORT virtual bool GetContactData(
        std::unique_ptr<proto::ContactData>& contactData) const = 0;
    OPENTXS_EXPORT virtual bool GetVerificationSet(
        std::unique_ptr<proto::VerificationSet>& verificationSet) const = 0;
    OPENTXS_EXPORT virtual bool hasCapability(
        const NymCapability& capability) const = 0;
    OPENTXS_EXPORT virtual Signature MasterSignature() const = 0;
    OPENTXS_EXPORT virtual proto::KeyMode Mode() const = 0;
    OPENTXS_EXPORT virtual identity::CredentialRole Role() const = 0;
    OPENTXS_EXPORT virtual bool Private() const = 0;
    OPENTXS_EXPORT virtual bool Save() const = 0;
    OPENTXS_EXPORT virtual Signature SelfSignature(
        CredentialModeFlag version = PUBLIC_VERSION) const = 0;
    OPENTXS_EXPORT virtual std::shared_ptr<SerializedType> Serialized(
        const SerializationModeFlag asPrivate,
        const SerializationSignatureFlag asSigned) const = 0;
    OPENTXS_EXPORT virtual Signature SourceSignature() const = 0;
    OPENTXS_EXPORT virtual bool TransportKey(
        Data& publicKey,
        Secret& privateKey,
        const PasswordPrompt& reason) const = 0;
    OPENTXS_EXPORT virtual identity::CredentialType Type() const = 0;
    OPENTXS_EXPORT virtual bool Verify(
        const Data& plaintext,
        const proto::Signature& sig,
        const proto::KeyRole key = proto::KEYROLE_SIGN) const = 0;
    OPENTXS_EXPORT virtual bool Verify(
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
