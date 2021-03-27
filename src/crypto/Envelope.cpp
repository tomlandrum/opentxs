// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"         // IWYU pragma: associated
#include "1_Internal.hpp"       // IWYU pragma: associated
#include "crypto/Envelope.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <iosfwd>
#include <iterator>
#include <map>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "2_Factory.hpp"
#include "internal/api/Api.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/crypto/Symmetric.hpp"
#include "opentxs/core/Armored.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/crypto/Envelope.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/identity/Authority.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/identity/Types.hpp"
#include "opentxs/protobuf/AsymmetricKey.pb.h"
#include "opentxs/protobuf/Ciphertext.pb.h"
#include "opentxs/protobuf/Enums.pb.h"
#include "opentxs/protobuf/Envelope.pb.h"
#include "opentxs/protobuf/TaggedKey.pb.h"

#define OT_METHOD "opentxs::crypto::implementation::Envelope::"

namespace opentxs
{
using ReturnType = crypto::implementation::Envelope;

auto Factory::Envelope(const api::internal::Core& api) noexcept
    -> std::unique_ptr<crypto::Envelope>
{
    return std::make_unique<ReturnType>(api);
}

auto Factory::Envelope(
    const api::internal::Core& api,
    const proto::Envelope& serialized) noexcept(false)
    -> std::unique_ptr<crypto::Envelope>
{
    return std::make_unique<ReturnType>(api, serialized);
}
}  // namespace opentxs

namespace opentxs::crypto::implementation
{
const VersionNumber Envelope::default_version_{2};
const VersionNumber Envelope::tagged_key_version_{1};
// NOTE: elements in supported_ must be added in sorted order or else
// test_solution() will not produce the correct result
const Envelope::SupportedKeys Envelope::supported_
{
#if OT_CRYPTO_SUPPORTED_KEY_RSA
    crypto::AsymmetricKeyType::Legacy,
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
        crypto::AsymmetricKeyType::Secp256k1,
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
#if OT_CRYPTO_SUPPORTED_KEY_ED25519
        crypto::AsymmetricKeyType::ED25519,
#endif  // OT_CRYPTO_SUPPORTED_KEY_ED25519
};
const Envelope::WeightMap Envelope::key_weights_{
    {crypto::AsymmetricKeyType::ED25519, 1},
    {crypto::AsymmetricKeyType::Secp256k1, 2},
    {crypto::AsymmetricKeyType::Legacy, 4},
};
const Envelope::Solutions Envelope::solutions_{calculate_solutions()};

Envelope::Envelope(const api::internal::Core& api) noexcept
    : api_(api)
    , version_(default_version_)
    , dh_keys_()
    , session_keys_()
    , ciphertext_()
{
}

Envelope::Envelope(
    const api::internal::Core& api,
    const SerializedType& in) noexcept(false)
    : api_(api)
    , version_(in.version())
    , dh_keys_(read_dh(api_, in))
    , session_keys_(read_sk(api_, in))
    , ciphertext_(read_ct(in))
{
}

Envelope::Envelope(const Envelope& rhs) noexcept
    : api_(rhs.api_)
    , version_(rhs.version_)
    , dh_keys_(clone(rhs.dh_keys_))
    , session_keys_(clone(rhs.session_keys_))
    , ciphertext_(clone(rhs.ciphertext_))
{
}

auto Envelope::Armored(opentxs::Armored& ciphertext) const noexcept -> bool
{
    return ciphertext.SetData(api_.Factory().Data(Serialize()));
}

auto Envelope::attach_session_keys(
    const identity::Nym& nym,
    const Solution& solution,
    const PasswordPrompt& previousPassword,
    const key::Symmetric& masterKey,
    const PasswordPrompt& reason) noexcept -> bool
{
    LogVerbose(OT_METHOD)(__FUNCTION__)(": Recipient ")(nym.ID())(" has ")(
        nym.size())(" master credentials")
        .Flush();

    for (const auto& authority : nym) {
        const auto type = solution.at(nym.ID()).at(authority.GetMasterCredID());
        auto tag = Tag{};
        auto password = api_.Factory().Secret(0);
        auto& dhKey = get_dh_key(type, authority, reason);
        const auto haveTag =
            dhKey.CalculateTag(authority, type, reason, tag, password);

        if (false == haveTag) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to calculate session password")
                .Flush();

            return false;
        }

        auto& key = std::get<2>(session_keys_.emplace_back(
                                    tag, type, OTSymmetricKey(masterKey)))
                        .get();
        const auto locked = key.ChangePassword(previousPassword, password);

        if (false == locked) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to lock session key")
                .Flush();

            return false;
        }
    }

    return true;
}

auto Envelope::calculate_requirements(const Nyms& recipients) noexcept(false)
    -> Requirements
{
    auto output = Requirements{};

    for (const auto& nym : recipients) {
        const auto& targets = output.emplace_back(nym->EncryptionTargets());

        if (targets.second.empty()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid recipient nym ")(
                nym->ID())
                .Flush();

            throw std::runtime_error("Invalid recipient nym");
        }
    }

    return output;
}

auto Envelope::calculate_solutions() noexcept -> Solutions
{
    constexpr auto one = std::size_t{1};
    auto output = Solutions{};

    for (auto row = one; row < (one << supported_.size()); ++row) {
        auto solution = std::pair<Weight, SupportedKeys>{};
        auto& [weight, keys] = solution;

        for (auto key = supported_.cbegin(); key != supported_.cend(); ++key) {
            const auto column = static_cast<std::size_t>(
                std::distance(supported_.cbegin(), key));

            if (0 != (row & (one << column))) { keys.emplace_back(*key); }
        }

        weight = std::accumulate(
            std::begin(keys),
            std::end(keys),
            0u,
            [](const auto& sum, const auto& key) -> Weight {
                return sum + key_weights_.at(key);
            });

        if (0 < weight) { output.emplace(std::move(solution)); }
    }

    return output;
}

auto Envelope::clone(const Ciphertext& rhs) noexcept -> Ciphertext
{
    if (rhs) { return std::make_unique<proto::Ciphertext>(*rhs); }

    return {};
}

auto Envelope::clone(const DHMap& rhs) noexcept -> DHMap
{
    auto output = DHMap{};

    for (const auto& [type, key] : rhs) { output.emplace(type, key); }

    return output;
}

auto Envelope::clone(const SessionKeys& rhs) noexcept -> SessionKeys
{
    auto output = SessionKeys{};

    for (const auto& [tag, type, key] : rhs) {
        output.emplace_back(tag, type, key);
    }

    return output;
}

auto Envelope::find_solution(const Nyms& recipients, Solution& map) noexcept
    -> SupportedKeys
{
    try {
        const auto requirements = calculate_requirements(recipients);

        for (const auto& [weight, keys] : solutions_) {
            if (test_solution(keys, requirements, map)) { return keys; }
        }
    } catch (...) {
    }

    return {};
}

auto Envelope::get_dh_key(
    const crypto::AsymmetricKeyType type,
    const identity::Authority& nym,
    const PasswordPrompt& reason) noexcept -> const key::Asymmetric&
{
    if (crypto::AsymmetricKeyType::Legacy != type) {
        const auto& set = dh_keys_.at(type);

        OT_ASSERT(1 == set.size());

        return *set.cbegin();
    } else {
#if OT_CRYPTO_SUPPORTED_KEY_RSA
        auto params = NymParameters{type};
        params.SetDHParams(nym.Params(type));
        auto& set = dh_keys_[type];
        set.emplace_back(api_.Factory().AsymmetricKey(
            params, reason, proto::KEYROLE_ENCRYPT));
        const auto& key = set.crbegin()->get();

        OT_ASSERT(key.keyType() == type);
        OT_ASSERT(key.Role() == proto::KEYROLE_ENCRYPT);
        OT_ASSERT(0 < key.Params().size());

        return key;
#else
        OT_FAIL;
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
    }
}

auto Envelope::Open(
    const identity::Nym& nym,
    const AllocateOutput plaintext,
    const PasswordPrompt& reason) const noexcept -> bool
{
    if (false == bool(ciphertext_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nothing to decrypt").Flush();

        return false;
    }

    const auto& ciphertext = *ciphertext_;

    try {
        auto password =
            api_.Factory().PasswordPrompt(reason.GetDisplayString());
        const auto& key = unlock_session_key(nym, password);

        return key.Decrypt(ciphertext, password, plaintext);
    } catch (...) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": No session keys for this nym")
            .Flush();

        return false;
    }
}

auto Envelope::read_dh(const api::Core& api, const SerializedType& rhs) noexcept
    -> DHMap
{
    auto output = DHMap{};

    try {
        for (const auto& key : rhs.dhkey()) {
            auto& set =
                output[crypto::key::Asymmetric::asymmetrickeytype_reverse_map_
                           .at(key.type())];
            set.emplace_back(api.Factory().AsymmetricKey(key));
        }
    } catch (...) {
    }

    return output;
}

auto Envelope::read_sk(const api::Core& api, const SerializedType& rhs) noexcept
    -> SessionKeys
{
    auto output = SessionKeys{};

    try {
        for (const auto& tagged : rhs.sessionkey()) {
            output.emplace_back(SessionKey{
                tagged.tag(),
                crypto::key::Asymmetric::asymmetrickeytype_reverse_map_.at(
                    tagged.type()),
                api.Symmetric().Key(
                    tagged.key(),
                    opentxs::crypto::SymmetricMode::ChaCha20Poly1305)});
        }
    } catch (...) {
    }

    return output;
}

auto Envelope::read_ct(const SerializedType& rhs) noexcept -> Ciphertext
{
    if (rhs.has_ciphertext()) {
        return std::make_unique<proto::Ciphertext>(rhs.ciphertext());
    }

    return {};
}

auto Envelope::Seal(
    const Recipients& recipients,
    const ReadView plaintext,
    const PasswordPrompt& reason) noexcept -> bool
{
    auto nyms = Nyms{};

    for (const auto& nym : recipients) {
        OT_ASSERT(nym);

        nyms.emplace_back(nym.get());
    }

    return seal(nyms, plaintext, reason);
}

auto Envelope::Seal(
    const identity::Nym& recipient,
    const ReadView plaintext,
    const PasswordPrompt& reason) noexcept -> bool
{
    return seal({&recipient}, plaintext, reason);
}

auto Envelope::seal(
    const Nyms recipients,
    const ReadView plaintext,
    const PasswordPrompt& reason) noexcept -> bool
{
    struct Cleanup {
        bool success_{false};

        Cleanup(Envelope& parent)
            : parent_(parent)
        {
        }

        ~Cleanup()
        {
            if (false == success_) {
                parent_.ciphertext_.reset();
                parent_.session_keys_.clear();
                parent_.dh_keys_.clear();
            }
        }

    private:
        Envelope& parent_;
    };

    if (ciphertext_) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Envelope has already been sealed")
            .Flush();

        return false;
    }

    if (0 == recipients.size()) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": No recipients").Flush();

        return false;
    } else {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": ")(recipients.size())(
            " recipient(s)")
            .Flush();
    }

    auto solution = Solution{};
    const auto dhkeys = find_solution(recipients, solution);

    if (0 == dhkeys.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": A recipient requires an unsupported key type")
            .Flush();

        return false;
    } else {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": ")(dhkeys.size())(
            " dhkeys will be created")
            .Flush();
    }

    auto cleanup = Cleanup(*this);

    for (const auto& type : dhkeys) {
        try {
            const auto params = NymParameters{type};

            if (crypto::AsymmetricKeyType::Legacy != type) {
                auto& set = dh_keys_[type];
                set.emplace_back(api_.Factory().AsymmetricKey(
                    params, reason, identity::KeyRole::Encrypt));
                const auto& key = set.crbegin()->get();

                OT_ASSERT(key.keyType() == type);
                OT_ASSERT(key.Role() == identity::KeyRole::Encrypt);
            }
        } catch (...) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to generate DH key")
                .Flush();

            return false;
        }
    }

    auto password = OTPasswordPrompt{reason};
    set_default_password(api_, password);
    auto masterKey = api_.Symmetric().Key(password);
    ciphertext_ = std::make_unique<proto::Ciphertext>();

    OT_ASSERT(ciphertext_);

    const auto encrypted =
        masterKey->Encrypt(plaintext, password, *ciphertext_, false);

    if (false == encrypted) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to encrypt plaintext")
            .Flush();

        return false;
    }

    for (const auto& nym : recipients) {
        if (false ==
            attach_session_keys(*nym, solution, password, masterKey, reason)) {
            return false;
        }
    }

    cleanup.success_ = true;

    return cleanup.success_;
}

auto Envelope::set_default_password(
    const api::Core& api,
    PasswordPrompt& password) noexcept -> bool
{
    return password.SetPassword(api.Factory().SecretFromText("opentxs"));
}

auto Envelope::Serialize() const noexcept -> SerializedType
{
    auto output = SerializedType{};
    output.set_version(version_);

    for (const auto& [type, set] : dh_keys_) {
        for (const auto& key : set) {
            *output.add_dhkey() = *key->asPublic()->Serialize();
        }
    }

    for (const auto& [tag, type, key] : session_keys_) {
        auto& tagged = *output.add_sessionkey();
        tagged.set_version(tagged_key_version_);
        tagged.set_tag(tag);
        tagged.set_type(
            crypto::key::Asymmetric::asymmetrickeytype_map_.at(type));
        key->Serialize(*tagged.mutable_key());
    }

    if (ciphertext_) { *output.mutable_ciphertext() = *ciphertext_; }

    return output;
}

auto Envelope::test_solution(
    const SupportedKeys& solution,
    const Requirements& requirements,
    Solution& map) noexcept -> bool
{
    map.clear();

    for (const auto& [nymID, credentials] : requirements) {
        auto& row = map.emplace(nymID, Solution::mapped_type{}).first->second;

        for (const auto& [credID, keys] : credentials) {
            if (0 == keys.size()) { return false; }

            auto test = SupportedKeys{};
            std::set_intersection(
                std::begin(solution),
                std::end(solution),
                std::begin(keys),
                std::end(keys),
                std::back_inserter(test));

            if (test.size() != keys.size()) { return false; }

            row.emplace(credID, *keys.cbegin());
        }
    }

    return true;
}

auto Envelope::unlock_session_key(
    const identity::Nym& nym,
    PasswordPrompt& reason) const noexcept(false) -> const key::Symmetric&
{
    for (const auto& [tag, type, key] : session_keys_) {
        try {
            for (const auto& dhKey : dh_keys_.at(type)) {
                if (nym.Unlock(dhKey, tag, type, key, reason)) { return key; }
            }
        } catch (...) {
            throw std::runtime_error("Missing dhkey");
        }
    }

    throw std::runtime_error("No session key usable by this nym");
}
}  // namespace opentxs::crypto::implementation
