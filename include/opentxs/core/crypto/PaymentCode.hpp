// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CRYPTO_PAYMENTCODE_HPP
#define OPENTXS_CORE_CRYPTO_PAYMENTCODE_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "opentxs/Bytes.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/crypto/Types.hpp"

namespace opentxs
{
namespace crypto
{
namespace key
{
class Asymmetric;
class EllipticCurve;
class HD;
}  // namespace key
}  // namespace crypto

namespace identity
{
namespace credential
{
class Base;
}  // namespace credential
}  // namespace identity

namespace identifier
{
class Nym;
}  // namespace identifier

namespace proto
{
class Credential;
class PaymentCode;
class Signature;
}  // namespace proto

class Data;
class PaymentCode;
class PasswordPrompt;

using OTPaymentCode = Pimpl<PaymentCode>;
}  // namespace opentxs

namespace opentxs
{
class OPENTXS_EXPORT PaymentCode
{
public:
    using ECKey = std::unique_ptr<crypto::key::EllipticCurve>;
    using HDKey = std::shared_ptr<crypto::key::HD>;
    using Serialized = proto::PaymentCode;
    using Elements = std::vector<Space>;

    static const VersionNumber DefaultVersion;

    virtual operator const crypto::key::Asymmetric&() const noexcept = 0;

    virtual bool operator==(const proto::PaymentCode& rhs) const noexcept = 0;

    virtual std::string asBase58() const noexcept = 0;
    virtual bool Blind(
        const PaymentCode& recipient,
        const crypto::key::EllipticCurve& privateKey,
        const ReadView outpoint,
        const AllocateOutput destination,
        const PasswordPrompt& reason) const noexcept = 0;
    virtual bool BlindV3(
        const PaymentCode& recipient,
        const crypto::key::EllipticCurve& privateKey,
        const AllocateOutput destination,
        const PasswordPrompt& reason) const noexcept = 0;
    virtual std::unique_ptr<PaymentCode> DecodeNotificationElements(
        const std::uint8_t version,
        const Elements& elements,
        const PasswordPrompt& reason) const noexcept = 0;
    virtual Elements GenerateNotificationElements(
        const PaymentCode& recipient,
        const crypto::key::EllipticCurve& privateKey,
        const PasswordPrompt& reason) const noexcept = 0;
    virtual const identifier::Nym& ID() const noexcept = 0;
    virtual HDKey Key() const noexcept = 0;
    virtual ECKey Incoming(
        const PaymentCode& sender,
        const Bip32Index index,
        const blockchain::Type chain,
        const PasswordPrompt& reason,
        const std::uint8_t version = 0) const noexcept = 0;
    virtual bool Locator(
        const AllocateOutput destination,
        const std::uint8_t version = 0) const noexcept = 0;
    virtual ECKey Outgoing(
        const PaymentCode& recipient,
        const Bip32Index index,
        const blockchain::Type chain,
        const PasswordPrompt& reason,
        const std::uint8_t version = 0) const noexcept = 0;
    virtual bool Serialize(
        AllocateOutput destination) const noexcept = 0;
    virtual Serialized Serialize() const noexcept = 0;
    virtual bool Sign(
        const identity::credential::Base& credential,
        proto::Signature& sig,
        const PasswordPrompt& reason) const noexcept = 0;
    virtual bool Sign(
        const Data& data,
        Data& output,
        const PasswordPrompt& reason) const noexcept = 0;
    virtual std::unique_ptr<PaymentCode> Unblind(
        const ReadView blinded,
        const crypto::key::EllipticCurve& publicKey,
        const ReadView outpoint,
        const PasswordPrompt& reason) const noexcept = 0;
    virtual std::unique_ptr<PaymentCode> UnblindV3(
        const std::uint8_t version,
        const ReadView blinded,
        const crypto::key::EllipticCurve& publicKey,
        const PasswordPrompt& reason) const noexcept = 0;
    virtual bool Valid() const noexcept = 0;
    virtual bool Verify(
        const proto::Credential& master,
        const proto::Signature& sourceSignature) const noexcept = 0;
    virtual VersionNumber Version() const noexcept = 0;

    virtual bool AddPrivateKeys(
        std::string& seed,
        const Bip32Index index,
        const PasswordPrompt& reason) noexcept = 0;

    virtual ~PaymentCode() = default;

protected:
    PaymentCode() = default;

private:
    friend OTPaymentCode;

    virtual PaymentCode* clone() const = 0;

    PaymentCode(const PaymentCode&) = delete;
    PaymentCode(PaymentCode&&) = delete;
    PaymentCode& operator=(const PaymentCode&);
    PaymentCode& operator=(PaymentCode&&);
};
}  // namespace opentxs
#endif
