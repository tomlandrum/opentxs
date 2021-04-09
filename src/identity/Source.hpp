// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>
#include <memory>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"
#include "opentxs/identity/Source.hpp"
#include "opentxs/identity/SourceType.hpp"
#include "opentxs/protobuf/Enums.pb.h"

namespace opentxs
{
namespace api
{
class Factory;
}  // namespace api

namespace identity
{
namespace credential
{
class Primary;
}  // namespace credential
}  // namespace identity

namespace proto
{
class AsymmetricKey;
class Credential;
class NymIDSource;
class Signature;
}  // namespace proto

class Factory;
class NymParameters;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::identity::implementation
{
class Source final : virtual public identity::Source
{
public:
    auto asString() const noexcept -> OTString final;
    auto Description() const noexcept -> OTString final;
    auto Type() const noexcept -> identity::SourceType final { return type_; }
    auto NymID() const noexcept -> OTNymID final;
    auto Serialize() const noexcept
        -> std::shared_ptr<proto::NymIDSource> final;
    auto Verify(
        const proto::Credential& master,
        const proto::Signature& sourceSignature) const noexcept -> bool final;
    auto Sign(
        const identity::credential::Primary& credential,
        proto::Signature& sig,
        const PasswordPrompt& reason) const noexcept -> bool final;

private:
    friend opentxs::Factory;

    using SourceTypeMap = std::map<identity::SourceType, proto::SourceType>;
    using SourceTypeReverseMap =
        std::map<proto::SourceType, identity::SourceType>;

    static const VersionConversionMap key_to_source_version_;

    const api::Factory& factory_;

    identity::SourceType type_;
    OTAsymmetricKey pubkey_;
    OTPaymentCode payment_code_;
    VersionNumber version_;

    static auto deserialize_pubkey(
        const api::Factory& factory,
        const identity::SourceType type,
        const proto::NymIDSource& serialized) -> OTAsymmetricKey;
    static auto deserialize_paymentcode(
        const api::Factory& factory,
        const identity::SourceType type,
        const proto::NymIDSource& serialized) -> OTPaymentCode;
    static auto extract_key(
        const proto::Credential& credential,
        const proto::KeyRole role) -> std::unique_ptr<proto::AsymmetricKey>;
    static auto sourcetype_map() noexcept -> const SourceTypeMap&;
    static auto translate(const identity::SourceType in) noexcept
        -> proto::SourceType;
    static auto translate(const proto::SourceType in) noexcept
        -> identity::SourceType;

    auto asData() const -> OTData;

    Source(
        const api::Factory& factory,
        const proto::NymIDSource& serializedSource) noexcept;
    Source(
        const api::Factory& factory,
        const NymParameters& nymParameters) noexcept(false);
    Source(const api::Factory& factory, const PaymentCode& source) noexcept;
    Source(const Source& rhs) noexcept;
    Source() = delete;
    Source(Source&&) = delete;
    auto operator=(const Source&) -> Source&;
    auto operator=(Source&&) -> Source&;
};
}  // namespace opentxs::identity::implementation
