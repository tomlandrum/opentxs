// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/Envelope.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/crypto/key/asymmetric/Algorithm.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/protobuf/Enums.pb.h"

namespace opentxs
{
namespace api
{
class Core;

namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace identity
{
class Authority;
}  // namespace identity

namespace proto
{
class Ciphertext;
}  // namespace proto

class Armored;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::crypto::implementation
{
class Envelope final : public crypto::Envelope
{
public:
    auto Armored(opentxs::Armored& ciphertext) const noexcept -> bool final;
    auto Open(
        const identity::Nym& recipient,
        const AllocateOutput plaintext,
        const PasswordPrompt& reason) const noexcept -> bool final;
    auto Serialize(AllocateOutput destination) const noexcept -> bool final;
    auto Serialize(SerializedType& serialized) const noexcept -> bool final;

    auto Seal(
        const Recipients& recipients,
        const ReadView plaintext,
        const PasswordPrompt& reason) noexcept -> bool final;
    auto Seal(
        const identity::Nym& theRecipient,
        const ReadView plaintext,
        const PasswordPrompt& reason) noexcept -> bool final;

    Envelope(const api::internal::Core& api) noexcept;
    Envelope(
        const api::internal::Core& api,
        const SerializedType& serialized) noexcept(false);
    Envelope(
        const api::internal::Core& api,
        const ReadView& serialized) noexcept(false);

    ~Envelope() final = default;

private:
    friend OTEnvelope;

    using Ciphertext = std::unique_ptr<proto::Ciphertext>;
    using DHMap = std::
        map<crypto::key::asymmetric::Algorithm, std::vector<OTAsymmetricKey>>;
    using Nyms = std::vector<const identity::Nym*>;
    using Tag = std::uint32_t;
    using SessionKey =
        std::tuple<Tag, crypto::key::asymmetric::Algorithm, OTSymmetricKey>;
    using SessionKeys = std::vector<SessionKey>;
    using SupportedKeys = std::vector<crypto::key::asymmetric::Algorithm>;
    using Weight = unsigned int;
    using WeightMap = std::map<crypto::key::asymmetric::Algorithm, Weight>;
    using Solution = std::map<
        OTNymID,
        std::map<OTIdentifier, crypto::key::asymmetric::Algorithm>>;
    using Solutions = std::map<Weight, SupportedKeys>;
    using Requirements = std::vector<identity::Nym::NymKeys>;

    static const VersionNumber default_version_;
    static const VersionNumber tagged_key_version_;
    static const SupportedKeys supported_;
    static const WeightMap key_weights_;
    static const Solutions solutions_;

    const api::internal::Core& api_;
    const VersionNumber version_;
    DHMap dh_keys_;
    SessionKeys session_keys_;
    Ciphertext ciphertext_;

    static auto calculate_requirements(const Nyms& recipients) noexcept(false)
        -> Requirements;
    static auto calculate_solutions() noexcept -> Solutions;
    static auto clone(const Ciphertext& rhs) noexcept -> Ciphertext;
    static auto clone(const DHMap& rhs) noexcept -> DHMap;
    static auto clone(const SessionKeys& rhs) noexcept -> SessionKeys;
    static auto find_solution(const Nyms& recipients, Solution& map) noexcept
        -> SupportedKeys;
    static auto read_dh(
        const api::Core& api,
        const SerializedType& rhs) noexcept -> DHMap;
    static auto read_sk(
        const api::Core& api,
        const SerializedType& rhs) noexcept -> SessionKeys;
    static auto read_ct(const SerializedType& rhs) noexcept -> Ciphertext;
    static auto set_default_password(
        const api::Core& api,
        PasswordPrompt& password) noexcept -> bool;
    static auto test_solution(
        const SupportedKeys& solution,
        const Requirements& requirements,
        Solution& map) noexcept -> bool;

    auto clone() const noexcept -> Envelope* final
    {
        return new Envelope{*this};
    }
    auto unlock_session_key(
        const identity::Nym& recipient,
        PasswordPrompt& reason) const noexcept(false) -> const key::Symmetric&;

    auto attach_session_keys(
        const identity::Nym& nym,
        const Solution& solution,
        const PasswordPrompt& previousPassword,
        const key::Symmetric& masterKey,
        const PasswordPrompt& reason) noexcept -> bool;
    auto get_dh_key(
        const crypto::key::asymmetric::Algorithm type,
        const identity::Authority& nym,
        const PasswordPrompt& reason) noexcept -> const key::Asymmetric&;
    auto seal(
        const Nyms recipients,
        const ReadView plaintext,
        const PasswordPrompt& reason) noexcept -> bool;

    Envelope() = delete;
    Envelope(const Envelope&) noexcept;
    Envelope(Envelope&&) = delete;
    auto operator=(const Envelope&) -> Envelope& = delete;
    auto operator=(Envelope&&) -> Envelope& = delete;
};
}  // namespace opentxs::crypto::implementation
