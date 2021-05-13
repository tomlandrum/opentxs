// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                       // IWYU pragma: associated
#include "1_Internal.hpp"                     // IWYU pragma: associated
#include "api/client/blockchain/Element.hpp"  // IWYU pragma: associated

#include <boost/container/vector.hpp>
#include <algorithm>
#include <chrono>
#include <iterator>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

#include "internal/api/Api.hpp"
#include "internal/api/client/Client.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/crypto/Asymmetric.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/crypto/key/EllipticCurve.hpp"
#include "opentxs/crypto/key/HD.hpp"  // IWYU pragma: keep
#include "opentxs/protobuf/AsymmetricKey.pb.h"
#include "opentxs/protobuf/BlockchainActivity.pb.h"
#include "opentxs/protobuf/BlockchainAddress.pb.h"

#define OT_METHOD "opentxs::api::client::blockchain::implementation::Element::"

namespace opentxs::api::client::blockchain::implementation
{
Element::Element(
    const api::internal::Core& api,
    const client::internal::Blockchain& blockchain,
    const internal::BalanceNode& parent,
    const opentxs::blockchain::Type chain,
    const VersionNumber version,
    const blockchain::Subchain subchain,
    const Bip32Index index,
    const std::string label,
    const OTIdentifier contact,
    const opentxs::crypto::key::EllipticCurve& key,
    const Time time,
    Transactions&& unconfirmed,
    Transactions&& confirmed) noexcept(false)
    : api_(api)
    , blockchain_(blockchain)
    , parent_(parent)
    , chain_(chain)
    , lock_()
    , version_(version)
    , subchain_(subchain)
    , index_(index)
    , label_(label)
    , contact_(contact)
    , pkey_(key.CloneEC())
    , timestamp_(time)
    , unconfirmed_(std::move(unconfirmed))
    , confirmed_(std::move(confirmed))
    , cached_(std::nullopt)
{
    if (false == bool(pkey_)) { throw std::runtime_error("No key provided"); }
}

Element::Element(
    const api::internal::Core& api,
    const client::internal::Blockchain& blockchain,
    const internal::BalanceNode& parent,
    const opentxs::blockchain::Type chain,
    const blockchain::Subchain subchain,
    const Bip32Index index,
    const opentxs::crypto::key::EllipticCurve& key) noexcept(false)
    : Element(
          api,
          blockchain,
          parent,
          chain,
          DefaultVersion,
          subchain,
          index,
          "",
          api.Factory().Identifier(),
          key,
          {},
          {},
          {})
{
}

Element::Element(
    const api::internal::Core& api,
    const client::internal::Blockchain& blockchain,
    const internal::BalanceNode& parent,
    const opentxs::blockchain::Type chain,
    const blockchain::Subchain subchain,
    const SerializedType& address) noexcept(false)
    : Element(
          api,
          blockchain,
          parent,
          chain,
          address.version(),
          subchain,
          address.index(),
          address.label(),
          api.Factory().Identifier(address.contact()),
          *instantiate(api, address.key()),
          Clock::from_time_t(address.modified()),
          [&] {
              auto out = Transactions{};

              for (const auto& txid : address.unconfirmed()) {
                  out.emplace(api.Factory().Data(txid, StringStyle::Raw));
              }

              return out;
          }(),
          [&] {
              auto out = Transactions{};

              for (const auto& txid : address.confirmed()) {
                  out.emplace(api.Factory().Data(txid, StringStyle::Raw));
              }

              return out;
          }())
{
    cached_ = address;
}

auto Element::Address(const AddressStyle format) const noexcept -> std::string
{
    auto lock = rLock{lock_};

    return blockchain_.CalculateAddress(
        chain_, format, api_.Factory().Data(pkey_->PublicKey()));
}

auto Element::Confirmed() const noexcept -> Txids
{
    auto lock = rLock{lock_};
    auto output = Txids{};
    std::copy(confirmed_.begin(), confirmed_.end(), std::back_inserter(output));

    return output;
}

auto Element::Confirm(const Txid& tx) noexcept -> bool
{
    if (tx.empty()) { return false; }

    auto lock = rLock{lock_};
    unconfirmed_.erase(tx);
    confirmed_.emplace(tx);
    timestamp_ = Clock::now();
    cached_ = std::nullopt;

    return true;
}

auto Element::Contact() const noexcept -> OTIdentifier
{
    auto lock = rLock{lock_};

    return contact_;
}

auto Element::Elements() const noexcept -> std::set<OTData>
{
    auto lock = rLock{lock_};

    return elements(lock);
}

auto Element::elements(const rLock&) const noexcept -> std::set<OTData>
{
    auto output = std::set<OTData>{};
    auto pubkey = api_.Factory().Data(pkey_->PublicKey());

    try {
        output.emplace(blockchain_.PubkeyHash(chain_, pubkey));
    } catch (...) {
        OT_FAIL;
    }

    return output;
}

auto Element::IncomingTransactions() const noexcept -> std::set<std::string>
{
    return parent_.IncomingTransactions(
        blockchain::Key{parent_.ID().str(), subchain_, index_});
}

auto Element::instantiate(
    const api::internal::Core& api,
    const proto::AsymmetricKey& serialized) noexcept(false)
    -> std::unique_ptr<opentxs::crypto::key::EllipticCurve>
{
    auto output = api.Asymmetric().InstantiateECKey(serialized);

    if (false == bool(output)) {
        throw std::runtime_error("Failed to construct key");
    }

    if (false == bool(*output)) { throw std::runtime_error("Wrong key type"); }

    return output;
}

auto Element::IsAvailable(const Identifier& contact, const std::string& memo)
    const noexcept -> Availability
{
    if (0 < confirmed_.size()) { return Availability::Used; }

    const auto age = std::chrono::duration_cast<std::chrono::hours>(
        Clock::now() - timestamp_);
    constexpr auto unconfirmedLimit = std::chrono::hours{24 * 7};
    constexpr auto reservedLimit = std::chrono::hours{24 * 2};
    const auto haveMetadata =
        ((false == contact_->empty()) || (false == label_.empty()));
    const auto match =
        haveMetadata && (contact_ == contact) && (label_ == memo);

    if (age > unconfirmedLimit) {
        if (match) {

            return Availability::Reissue;
        } else if (haveMetadata) {

            return Availability::MetadataConflict;
        } else if (0 < unconfirmed_.size()) {

            return Availability::StaleUnconfirmed;
        } else {

            return Availability::NeverUsed;
        }
    } else if (age > reservedLimit) {
        if (0 < unconfirmed_.size()) {

            return Availability::Reserved;
        } else if (match) {

            return Availability::Reissue;
        } else if (haveMetadata) {

            return Availability::MetadataConflict;
        } else {

            return Availability::NeverUsed;
        }
    } else {

        return Availability::Reserved;
    }
}

auto Element::Key() const noexcept -> ECKey
{
    auto lock = rLock{lock_};

    return pkey_;
}

auto Element::Label() const noexcept -> std::string
{
    auto lock = rLock{lock_};

    return label_;
}

auto Element::LastActivity() const noexcept -> Time
{
    auto lock = rLock{lock_};

    return timestamp_;
}

auto Element::PrivateKey(const PasswordPrompt& reason) const noexcept -> ECKey
{
    auto lock = rLock{lock_};

    if (false == pkey_->HasPrivate()) {
        auto key = parent_.PrivateKey(subchain_, index_, reason);

        if (!key) { return {}; }

        pkey_ = std::move(key);

        OT_ASSERT(pkey_);
    }

    OT_ASSERT(pkey_->HasPrivate());

    return pkey_;
}

auto Element::PubkeyHash() const noexcept -> OTData
{
    auto lock = rLock{lock_};
    const auto key = api_.Factory().Data(pkey_->PublicKey());

    return blockchain_.PubkeyHash(chain_, key);
}

auto Element::Reserve(const Time time) noexcept -> bool
{
    auto lock = rLock{lock_};
    timestamp_ = time;
    cached_ = std::nullopt;

    return true;
}

auto Element::Serialize() const noexcept -> Element::SerializedType
{
    auto lock = rLock{lock_};

    if (false == cached_.has_value()) {
        const auto key = [&] {
            auto serialized = proto::AsymmetricKey{};
            if (pkey_->HasPrivate()) {
                pkey_->asPublicEC()->Serialize(serialized);
            } else {
                pkey_->Serialize(serialized);
            }
            return serialized;
        }();

        auto& output = cached_.emplace();
        output.set_version(
            (DefaultVersion > version_) ? DefaultVersion : version_);
        output.set_index(index_);
        output.set_label(label_);
        output.set_contact(contact_->str());
        *output.mutable_key() = key;
        output.set_modified(Clock::to_time_t(timestamp_));

        for (const auto& txid : unconfirmed_) {
            output.add_unconfirmed(txid->str());
        }

        for (const auto& txid : confirmed_) {
            output.add_confirmed(txid->str());
        }
    }

    return cached_.value();
}

void Element::SetContact(const Identifier& contact) noexcept
{
    auto lock = rLock{lock_};
    contact_ = contact;
    cached_ = std::nullopt;
    update_element(lock);
}

void Element::SetLabel(const std::string& label) noexcept
{
    auto lock = rLock{lock_};
    label_ = label;
    cached_ = std::nullopt;
    update_element(lock);
}

void Element::SetMetadata(
    const Identifier& contact,
    const std::string& label) noexcept
{
    auto lock = rLock{lock_};
    contact_ = contact;
    label_ = label;
    cached_ = std::nullopt;
    update_element(lock);
}

auto Element::Unconfirm(const Txid& tx, const Time time) noexcept -> bool
{
    if (tx.empty()) { return false; }

    auto lock = rLock{lock_};
    confirmed_.erase(tx);
    unconfirmed_.emplace(tx);
    timestamp_ = time;
    cached_ = std::nullopt;

    return true;
}

auto Element::Unconfirmed() const noexcept -> Txids
{
    auto lock = rLock{lock_};
    auto output = Txids{};
    std::copy(
        unconfirmed_.begin(), unconfirmed_.end(), std::back_inserter(output));

    return output;
}

auto Element::Unreserve() noexcept -> bool
{
    auto lock = rLock{lock_};

    if ((0u < confirmed_.size()) || (0u < confirmed_.size())) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(
            ": element is already associated with transactions")
            .Flush();

        return false;
    }

    timestamp_ = {};
    label_ = {};
    contact_ = api_.Factory().Identifier();
    cached_ = std::nullopt;

    return true;
}

auto Element::update_element(rLock& lock) const noexcept -> void
{
    const auto elements = this->elements(lock);
    auto hashes = std::vector<ReadView>{};
    std::transform(
        std::begin(elements), std::end(elements), std::back_inserter(hashes), [
        ](const auto& in) -> auto { return in->Bytes(); });
    lock.unlock();
    parent_.UpdateElement(hashes);
}
}  // namespace opentxs::api::client::blockchain::implementation
