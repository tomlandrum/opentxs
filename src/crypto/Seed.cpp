// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"     // IWYU pragma: associated
#include "1_Internal.hpp"   // IWYU pragma: associated
#include "crypto/Seed.hpp"  // IWYU pragma: associated

#include <boost/container/flat_map.hpp>
#include <algorithm>
#include <functional>
#include <iosfwd>
#include <mutex>
#include <stdexcept>
#include <utility>

#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/crypto/Symmetric.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/crypto/Bip32.hpp"
#include "opentxs/crypto/Bip39.hpp"
#include "opentxs/crypto/Language.hpp"
#include "opentxs/crypto/SeedStrength.hpp"
#include "opentxs/crypto/SeedStyle.hpp"
#include "opentxs/crypto/SymmetricMode.hpp"
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/protobuf/Ciphertext.pb.h"
#include "opentxs/protobuf/Enums.pb.h"
#include "opentxs/protobuf/Seed.pb.h"
#include "util/Container.hpp"

#define OT_METHOD "opentxs::crypto::Seed::"

namespace opentxs::crypto
{
struct Seed::Imp {
    const SeedStyle type_;
    const Language lang_;
    const OTSecret entropy_;
    const OTSecret words_;
    const OTSecret phrase_;
    const OTIdentifier id_;

    auto Index() const noexcept -> Bip32Index
    {
        auto lock = Lock{lock_};

        return index_;
    }

    auto IncrementIndex(const Bip32Index index) noexcept -> bool
    {
        auto lock = Lock{lock_};

        if (index_ > index) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Index values must always increase.")
                .Flush();

            return false;
        }

        index_ = index;
        version_ = std::max(version_, default_version_);

        return save(lock);
    }

    Imp(const opentxs::crypto::Bip32& bip32,
        const opentxs::crypto::Bip39& bip39,
        const api::crypto::Symmetric& symmetric,
        const api::Factory& factory,
        const api::storage::Storage& storage,
        const Language lang,
        const SeedStrength strength,
        const PasswordPrompt& reason) noexcept(false)
        : type_(SeedStyle::BIP39)
        , lang_(lang)
        , entropy_([&] {
            auto out = factory.Secret(0);
            static constexpr auto bitsPerByte{8u};
            const auto bytes = static_cast<std::size_t>(strength) / bitsPerByte;

            if ((16u > bytes) || (32u < bytes)) {
                throw std::runtime_error{"Invalid seed strength"};
            }

            out->Randomize(static_cast<std::size_t>(strength) / bitsPerByte);

            return out;
        }())
        , words_([&] {
            auto out = factory.Secret(0);

            if (false == bip39.SeedToWords(entropy_, out, lang_)) {
                throw std::runtime_error{
                    "Unable to convert entropy to word list"};
            }

            return out;
        }())
        , phrase_([&] {
            auto out = factory.Secret(0);
            out->AssignText(no_passphrase_);

            return out;
        }())
        , id_(bip32.SeedID(entropy_->Bytes()))
        , storage_(storage)
        , lock_()
        , encrypted_words_()
        , encrypted_phrase_()
        , encrypted_entropy_(encrypt(
              type_,
              symmetric,
              entropy_,
              words_,
              phrase_,
              const_cast<proto::Ciphertext&>(encrypted_words_),
              const_cast<proto::Ciphertext&>(encrypted_phrase_),
              reason))
        , version_(default_version_)
        , index_(0)
    {
        if (16u > entropy_->size()) {
            throw std::runtime_error{"Entropy too short"};
        }

        if (64u < entropy_->size()) {
            throw std::runtime_error{"Entropy too long"};
        }

        auto lock = Lock{lock_};

        if (false == save(lock)) {
            throw std::runtime_error{"Failed to save seed"};
        }
    }
    Imp(const opentxs::crypto::Bip32& bip32,
        const opentxs::crypto::Bip39& bip39,
        const api::crypto::Symmetric& symmetric,
        const api::Factory& factory,
        const api::storage::Storage& storage,
        const SeedStyle type,
        const Language lang,
        const Secret& words,
        const Secret& passphrase,
        const PasswordPrompt& reason) noexcept(false)
        : type_(SeedStyle::BIP39)
        , lang_(lang)
        , entropy_([&] {
            auto out = factory.Secret(0);
            bip39.WordsToSeed(words, out, passphrase);

            return out;
        }())
        , words_(words)
        , phrase_(passphrase)
        , id_(bip32.SeedID(entropy_->Bytes()))
        , storage_(storage)
        , lock_()
        , encrypted_words_()
        , encrypted_phrase_()
        , encrypted_entropy_(encrypt(
              type_,
              symmetric,
              entropy_,
              words_,
              phrase_,
              const_cast<proto::Ciphertext&>(encrypted_words_),
              const_cast<proto::Ciphertext&>(encrypted_phrase_),
              reason))
        , version_(default_version_)
        , index_(0)
    {
        if (16u > entropy_->size()) {
            throw std::runtime_error{"Entropy too short"};
        }

        if (64u < entropy_->size()) {
            throw std::runtime_error{"Entropy too long"};
        }

        auto lock = Lock{lock_};

        if (false == save(lock)) {
            throw std::runtime_error{"Failed to save seed"};
        }
    }
    Imp(const opentxs::crypto::Bip32& bip32,
        const opentxs::crypto::Bip39& bip39,
        const api::crypto::Symmetric& symmetric,
        const api::Factory& factory,
        const api::storage::Storage& storage,
        const Secret& entropy,
        const PasswordPrompt& reason) noexcept(false)
        : type_(SeedStyle::BIP32)
        , lang_(Language::none)
        , entropy_(entropy)
        , words_(factory.Secret(0))
        , phrase_(factory.Secret(0))
        , id_(bip32.SeedID(entropy_->Bytes()))
        , storage_(storage)
        , lock_()
        , encrypted_words_()
        , encrypted_phrase_()
        , encrypted_entropy_(encrypt(
              type_,
              symmetric,
              entropy_,
              words_,
              phrase_,
              const_cast<proto::Ciphertext&>(encrypted_words_),
              const_cast<proto::Ciphertext&>(encrypted_phrase_),
              reason))
        , version_(default_version_)
        , index_(0)
    {
        if (16u > entropy_->size()) {
            throw std::runtime_error{"Entropy too short"};
        }

        if (64u < entropy_->size()) {
            throw std::runtime_error{"Entropy too long"};
        }

        auto lock = Lock{lock_};

        if (false == save(lock)) {
            throw std::runtime_error{"Failed to save seed"};
        }
    }
    Imp(const opentxs::crypto::Bip39& bip39,
        const api::crypto::Symmetric& symmetric,
        const api::Factory& factory,
        const api::storage::Storage& storage,
        const proto::Seed& proto,
        const PasswordPrompt& reason) noexcept(false)
        : type_(translate(proto.type()))
        , lang_(translate(proto.lang()))
        , entropy_(factory.Secret(0))
        , words_(factory.Secret(0))
        , phrase_(factory.Secret(0))
        , id_(factory.Identifier(proto.fingerprint()))
        , storage_(storage)
        , lock_()
        , encrypted_words_(
              proto.has_words() ? proto.words() : proto::Ciphertext{})
        , encrypted_phrase_(
              proto.has_passphrase() ? proto.passphrase() : proto::Ciphertext{})
        , encrypted_entropy_(
              proto.has_raw() ? proto.raw() : proto::Ciphertext{})
        , version_(proto.version())
        , index_(proto.index())
    {
        const auto& session =
            (3 > version_) ? encrypted_words_ : encrypted_entropy_;
        const auto key =
            symmetric.Key(session.key(), translate(session.mode()));

        if (false == key.get()) {
            throw std::runtime_error{"Failed to get decryption key"};
        }

        if (proto.has_words()) {
            auto& words = const_cast<Secret&>(words_.get());
            const auto rc = key->Decrypt(
                encrypted_words_, reason, words.WriteInto(Secret::Mode::Text));

            if (false == rc) {
                throw std::runtime_error{"Failed to decrypt words"};
            }
        }

        if (proto.has_passphrase()) {
            auto& phrase = const_cast<Secret&>(phrase_.get());
            const auto rc = key->Decrypt(
                encrypted_phrase_,
                reason,
                phrase.WriteInto(Secret::Mode::Text));

            if (false == rc) {
                throw std::runtime_error{"Failed to decrypt passphrase"};
            }
        }

        if (proto.has_raw()) {
            auto& entropy = const_cast<Secret&>(entropy_.get());
            const auto rc = key->Decrypt(
                encrypted_entropy_,
                reason,
                entropy.WriteInto(Secret::Mode::Text));

            if (false == rc) {
                throw std::runtime_error{"Failed to decrypt entropy"};
            }
        } else {
            OT_ASSERT(proto.has_words());

            auto& entropy = const_cast<Secret&>(entropy_.get());
            bip39.WordsToSeed(words_, entropy, phrase_);
            auto ctext = const_cast<proto::Ciphertext&>(encrypted_entropy_);
            auto cwords = const_cast<proto::Ciphertext&>(encrypted_words_);

            if (!key->Encrypt(entropy_->Bytes(), reason, ctext, true)) {
                throw std::runtime_error{"Failed to encrypt entropy"};
            }

            if (!key->Encrypt(words_->Bytes(), reason, cwords, false)) {
                throw std::runtime_error{"Failed to encrypt words"};
            }

            version_ = default_version_;
        }
    }

private:
    using SerializeType = proto::Seed;
    using SymmetricModeMap =
        boost::container::flat_map<crypto::SymmetricMode, proto::SymmetricMode>;
    using SymmetricModeReverseMap =
        boost::container::flat_map<proto::SymmetricMode, crypto::SymmetricMode>;
    using TypeMap = boost::container::flat_map<SeedStyle, proto::SeedType>;
    using TypeReverseMap =
        boost::container::flat_map<proto::SeedType, SeedStyle>;
    using LangMap = boost::container::flat_map<Language, proto::SeedLang>;
    using LangReverseMap =
        boost::container::flat_map<proto::SeedLang, Language>;

    static constexpr auto default_version_ = VersionNumber{4u};
    static constexpr auto no_passphrase_{""};

    const api::storage::Storage& storage_;
    mutable std::mutex lock_;
    const proto::Ciphertext encrypted_words_;
    const proto::Ciphertext encrypted_phrase_;
    const proto::Ciphertext encrypted_entropy_;
    VersionNumber version_;
    Bip32Index index_;

    static auto encrypt(
        const SeedStyle type,
        const api::crypto::Symmetric& symmetric,
        const Secret& entropy,
        const Secret& words,
        const Secret& phrase,
        proto::Ciphertext& cwords,
        proto::Ciphertext& cphrase,
        const PasswordPrompt& reason) noexcept(false) -> proto::Ciphertext
    {
        auto key =
            symmetric.Key(reason, crypto::SymmetricMode::ChaCha20Poly1305);

        if (false == key.get()) {
            throw std::runtime_error{"Failed to get encryption key"};
        }

        if (0u < words.size()) {
            if (!key->Encrypt(words.Bytes(), reason, cwords, false)) {
                throw std::runtime_error{"Failed to encrypt words"};
            }
        }

        if (0u < phrase.size()) {
            if (!key->Encrypt(phrase.Bytes(), reason, cphrase, false)) {
                throw std::runtime_error{"Failed to encrypt phrase"};
            }
        }

        auto out = proto::Ciphertext{};

        if (!key->Encrypt(entropy.Bytes(), reason, out, true)) {
            throw std::runtime_error{"Failed to encrypt entropy"};
        }

        return out;
    }
    static auto lang_map() noexcept -> const LangMap&
    {
        static const auto map = LangMap{
            {Language::none, proto::SEEDLANG_NONE},
            {Language::en, proto::SEEDLANG_EN},
        };

        return map;
    }
    static auto symmetricmode_map() noexcept -> const SymmetricModeMap&
    {
        static const auto map = SymmetricModeMap{
            {SymmetricMode::Error, proto::SMODE_ERROR},
            {SymmetricMode::ChaCha20Poly1305, proto::SMODE_CHACHA20POLY1305},
        };

        return map;
    }
    static auto translate(const SeedStyle in) noexcept -> proto::SeedType
    {
        try {

            return type_map().at(in);
        } catch (...) {

            return proto::SEEDTYPE_ERROR;
        }
    }
    static auto translate(const proto::SeedType in) noexcept -> SeedStyle
    {
        static const auto map =
            reverse_arbitrary_map<SeedStyle, proto::SeedType, TypeReverseMap>(
                type_map());

        try {

            return map.at(in);
        } catch (...) {

            return SeedStyle::Error;
        }
    }
    static auto translate(const Language in) noexcept -> proto::SeedLang
    {
        try {

            return lang_map().at(in);
        } catch (...) {

            return proto::SEEDLANG_NONE;
        }
    }
    static auto translate(const proto::SeedLang in) noexcept -> Language
    {
        static const auto map =
            reverse_arbitrary_map<Language, proto::SeedLang, LangReverseMap>(
                lang_map());

        try {

            return map.at(in);
        } catch (...) {

            return Language::none;
        }
    }
    static auto translate(const SymmetricMode in) noexcept
        -> proto::SymmetricMode
    {
        try {

            return symmetricmode_map().at(in);
        } catch (...) {

            return proto::SMODE_ERROR;
        }
    }
    static auto translate(const proto::SymmetricMode in) noexcept
        -> crypto::SymmetricMode
    {
        static const auto map = reverse_arbitrary_map<
            crypto::SymmetricMode,
            proto::SymmetricMode,
            SymmetricModeReverseMap>(symmetricmode_map());

        try {

            return map.at(in);
        } catch (...) {

            return SymmetricMode::Error;
        }
    }
    static auto type_map() noexcept -> const TypeMap&
    {
        static const auto map = TypeMap{
            {SeedStyle::BIP32, proto::SEEDTYPE_RAW},
            {SeedStyle::BIP39, proto::SEEDTYPE_BIP39},
        };

        return map;
    }

    auto save(const Lock& lock) const noexcept -> bool
    {
        const auto id = id_->str();
        auto proto = SerializeType{};
        proto.set_version(version_);
        proto.set_index(index_);
        proto.set_fingerprint(id);
        proto.set_type(translate(type_));
        proto.set_lang(translate(lang_));

        if (0u < words_->size()) { *proto.mutable_words() = encrypted_words_; }

        if (0u < phrase_->size()) {
            *proto.mutable_passphrase() = encrypted_phrase_;
        }

        *proto.mutable_raw() = encrypted_entropy_;

        if (false == storage_.Store(proto, id)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to store seed.")
                .Flush();

            return false;
        }

        return true;
    }
};

Seed::Seed(
    const opentxs::crypto::Bip32& bip32,
    const opentxs::crypto::Bip39& bip39,
    const api::crypto::Symmetric& symmetric,
    const api::Factory& factory,
    const api::storage::Storage& storage,
    const Language lang,
    const SeedStrength strength,
    const PasswordPrompt& reason) noexcept(false)
    : imp_(std::make_unique<Imp>(
          bip32,
          bip39,
          symmetric,
          factory,
          storage,
          lang,
          strength,
          reason))
{
    OT_ASSERT(imp_);
}

Seed::Seed(
    const opentxs::crypto::Bip32& bip32,
    const opentxs::crypto::Bip39& bip39,
    const api::crypto::Symmetric& symmetric,
    const api::Factory& factory,
    const api::storage::Storage& storage,
    const SeedStyle type,
    const Language lang,
    const Secret& words,
    const Secret& passphrase,
    const PasswordPrompt& reason) noexcept(false)
    : imp_(std::make_unique<Imp>(
          bip32,
          bip39,
          symmetric,
          factory,
          storage,
          type,
          lang,
          words,
          passphrase,
          reason))
{
    OT_ASSERT(imp_);
}

Seed::Seed(
    const opentxs::crypto::Bip32& bip32,
    const opentxs::crypto::Bip39& bip39,
    const api::crypto::Symmetric& symmetric,
    const api::Factory& factory,
    const api::storage::Storage& storage,
    const Secret& entropy,
    const PasswordPrompt& reason) noexcept(false)
    : imp_(std::make_unique<
           Imp>(bip32, bip39, symmetric, factory, storage, entropy, reason))
{
    OT_ASSERT(imp_);
}

Seed::Seed(
    const opentxs::crypto::Bip39& bip39,
    const api::crypto::Symmetric& symmetric,
    const api::Factory& factory,
    const api::storage::Storage& storage,
    const proto::Seed& proto,
    const PasswordPrompt& reason) noexcept(false)
    : imp_(std::make_unique<
           Imp>(bip39, symmetric, factory, storage, proto, reason))
{
    OT_ASSERT(imp_);
}

Seed::Seed(Seed&& rhs) noexcept
    : imp_(std::move(rhs.imp_))
{
    OT_ASSERT(imp_);
}

auto Seed::Entropy() const noexcept -> const Secret& { return imp_->entropy_; }

auto Seed::ID() const noexcept -> const Identifier& { return imp_->id_; }

auto Seed::IncrementIndex(const Bip32Index index) noexcept -> bool
{
    return imp_->IncrementIndex(index);
}

auto Seed::Index() const noexcept -> Bip32Index { return imp_->Index(); }

auto Seed::Phrase() const noexcept -> const Secret& { return imp_->phrase_; }

auto Seed::Words() const noexcept -> const Secret& { return imp_->words_; }

Seed::~Seed() = default;
}  // namespace opentxs::crypto
