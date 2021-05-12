// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                           // IWYU pragma: associated
#include "1_Internal.hpp"                         // IWYU pragma: associated
#include "api/client/blockchain/PaymentCode.hpp"  // IWYU pragma: associated

#include <robin_hood.h>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "api/client/blockchain/Deterministic.hpp"
#include "api/client/blockchain/Element.hpp"
#include "internal/api/Api.hpp"
#include "internal/api/client/Client.hpp"
#include "internal/api/client/blockchain/Factory.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/client/blockchain/BalanceNodeType.hpp"
#include "opentxs/api/client/blockchain/Subchain.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/key/HD.hpp"
#include "opentxs/protobuf/Bip47Channel.pb.h"
#include "opentxs/protobuf/Bip47Direction.pb.h"
#include "opentxs/protobuf/BlockchainAddress.pb.h"
#include "opentxs/protobuf/HDPath.pb.h"
#include "opentxs/protobuf/PaymentCode.pb.h"

#define OT_METHOD                                                              \
    "opentxs::api::client::blockchain::implementation::PaymentCode::"

namespace opentxs::factory
{
using ReturnType = api::client::blockchain::implementation::PaymentCode;

auto BlockchainPCBalanceNode(
    const api::internal::Core& api,
    const api::client::blockchain::internal::BalanceTree& parent,
    const opentxs::PaymentCode& local,
    const opentxs::PaymentCode& remote,
    const proto::HDPath& path,
    const Data& txid,
    const PasswordPrompt& reason,
    Identifier& id) noexcept
    -> std::unique_ptr<api::client::blockchain::internal::PaymentCode>
{
    try {
        return std::make_unique<ReturnType>(
            api, parent, local, remote, path, txid, reason, id);
    } catch (const std::exception& e) {
        LogVerbose("opentxs::Factory::")(__FUNCTION__)(": ")(e.what()).Flush();

        return nullptr;
    }
}

auto BlockchainPCBalanceNode(
    const api::internal::Core& api,
    const api::client::blockchain::internal::BalanceTree& parent,
    const proto::Bip47Channel& serialized,
    Identifier& id) noexcept
    -> std::unique_ptr<api::client::blockchain::internal::PaymentCode>
{
    using ReturnType = api::client::blockchain::implementation::PaymentCode;

    try {
        return std::make_unique<ReturnType>(api, parent, serialized, id);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": ")(e.what()).Flush();

        return nullptr;
    }
}
}  // namespace opentxs::factory

namespace opentxs::api::client::blockchain::internal
{
auto PaymentCode::GetID(
    const api::Core& api,
    const Chain chain,
    const opentxs::PaymentCode& local,
    const opentxs::PaymentCode& remote) noexcept -> OTIdentifier
{
    auto out = api.Factory().Identifier();
    auto preimage = api.Factory().Data();
    preimage->Assign(&chain, sizeof(chain));
    preimage->Concatenate(local.ID().Bytes());
    preimage->Concatenate(remote.ID().Bytes());
    out->CalculateDigest(preimage->Bytes());

    return out;
}
}  // namespace opentxs::api::client::blockchain::internal

namespace opentxs::api::client::blockchain::implementation
{
constexpr auto internalType{Subchain::Outgoing};
constexpr auto externalType{Subchain::Incoming};

PaymentCode::PaymentCode(
    const api::internal::Core& api,
    const internal::BalanceTree& parent,
    const opentxs::PaymentCode& local,
    const opentxs::PaymentCode& remote,
    const proto::HDPath& path,
    const opentxs::blockchain::block::Txid& txid,
    const PasswordPrompt& reason,
    Identifier& id) noexcept(false)
    : Deterministic(
          api,
          parent,
          BalanceNodeType::PaymentCode,
          internal::PaymentCode::GetID(api, parent.Chain(), local, remote),
          path,
          {{internalType, false, {}}, {externalType, false, {}}},
          id)
    , version_(DefaultVersion)
    , outgoing_notifications_()
    , incoming_notifications_([&] {
        auto out = std::set<opentxs::blockchain::block::pTxid>{};

        if (false == txid.empty()) { out.emplace(txid); }

        return out;
    }())
    , local_(local, compare_)
    , remote_(remote, compare_)
{
    const auto test_path = [&] {
        auto seed{path_.root()};

        return local_.get().AddPrivateKeys(
            seed, *path_.child().rbegin(), reason);
    };

    if (false == test_path()) {
        throw std::runtime_error("Invalid path or local payment code");
    }

    init(reason);
}

PaymentCode::PaymentCode(
    const api::internal::Core& api,
    const internal::BalanceTree& parent,
    const SerializedType& serialized,
    Identifier& id) noexcept(false)
    : Deterministic(
          api,
          parent,
          BalanceNodeType::PaymentCode,
          serialized.deterministic(),
          serialized.incoming().address().size(),
          serialized.outgoing().address().size(),
          [&] {
              auto out = ChainData{
                  {internalType, false, {}}, {externalType, false, {}}};
              auto& internal = out.internal_.map_;
              auto& external = out.external_.map_;
              internal.reserve(serialized.outgoing().address().size());
              external.reserve(serialized.incoming().address().size());

              for (const auto& address : serialized.outgoing().address()) {
                  internal.emplace(
                      std::piecewise_construct,
                      std::forward_as_tuple(address.index()),
                      std::forward_as_tuple(
                          std::make_unique<implementation::Element>(
                              api,
                              parent.Parent().Parent(),
                              *this,
                              parent.Chain(),
                              internalType,
                              address)));
              }

              for (const auto& address : serialized.incoming().address()) {
                  external.emplace(
                      std::piecewise_construct,
                      std::forward_as_tuple(address.index()),
                      std::forward_as_tuple(
                          std::make_unique<implementation::Element>(
                              api,
                              parent.Parent().Parent(),
                              *this,
                              parent.Chain(),
                              data_.external_.type_,
                              address)));
              }

              return out;
          }(),
          id)
    , version_(serialized.version())
    , outgoing_notifications_([&] {
        auto out = std::set<opentxs::blockchain::block::pTxid>{};

        for (const auto& notif : serialized.outgoing().notification()) {
            out.emplace(api_.Factory().Data(notif, StringStyle::Raw));
        }

        return out;
    }())
    , incoming_notifications_([&] {
        auto out = std::set<opentxs::blockchain::block::pTxid>{};

        for (const auto& notif : serialized.incoming().notification()) {
            out.emplace(api_.Factory().Data(notif, StringStyle::Raw));
        }

        return out;
    }())
    , local_(api_.Factory().PaymentCode(serialized.local()), compare_)
    , remote_(api_.Factory().PaymentCode(serialized.remote()), compare_)
{
    init();
}

auto PaymentCode::account_already_exists(const rLock&) const noexcept -> bool
{
    const auto existing =
        api_.Storage().Bip47ChannelsByChain(parent_.NymID(), Translate(chain_));

    return 0 < existing.count(id_);
}

auto PaymentCode::AddNotification(const Txid& tx) const noexcept -> bool
{
    auto lock = rLock{lock_};

    if (0 < outgoing_notifications_.count(tx)) { return true; }

    outgoing_notifications_.emplace(tx);
    const auto out = save(lock);

    if (false == out) { outgoing_notifications_.erase(tx); }

    return out;
}

auto PaymentCode::get_contact() const noexcept -> OTIdentifier
{
    // TODO works for now but the Contacts api needs a better way to
    // map payment codes to contacts since contacts might have supplemental
    // payment codes
    return parent_.Parent().Parent().Contacts().NymToContact(
        remote_.get().ID());
}

auto PaymentCode::has_private(const PasswordPrompt& reason) const noexcept
    -> bool
{
    auto pKey = local_.get().Key();

    if (!pKey) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": No local HD key").Flush();

        return false;
    }

    auto& key = *pKey;

    if (key.HasPrivate()) { return true; }

    auto seed{path_.root()};

    return local_.get().AddPrivateKeys(seed, *path_.child().rbegin(), reason);
}

auto PaymentCode::IsNotified() const noexcept -> bool
{
    auto lock = rLock{lock_};

    return 0 < outgoing_notifications_.size();
}

auto PaymentCode::PrivateKey(
    const Subchain type,
    const Bip32Index index,
    const PasswordPrompt& reason) const noexcept -> ECKey
{
    if (false == has_private(reason)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing private key").Flush();

        return {};
    }

    switch (type) {
        case internalType: {
            return local_.get().Outgoing(remote_, index, chain_, reason);
        }
        case externalType: {
            return local_.get().Incoming(remote_, index, chain_, reason);
        }
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid subchain").Flush();

            return {};
        }
    }
}

auto PaymentCode::ReorgNotification(const Txid& tx) const noexcept -> bool
{
    auto lock = rLock{lock_};

    if (0 == outgoing_notifications_.count(tx)) { return true; }

    outgoing_notifications_.erase(tx);
    const auto out = save(lock);

    if (false == out) { outgoing_notifications_.emplace(tx); }

    return out;
}

auto PaymentCode::Reserve(
    const Subchain type,
    const std::size_t batch,
    const PasswordPrompt& reason,
    const Identifier&,
    const std::string& label,
    const Time time) const noexcept -> Batch
{
    return Deterministic::Reserve(
        type, batch, reason, get_contact(), label, time);
}

auto PaymentCode::save(const rLock& lock) const noexcept -> bool
{
    auto serialized = SerializedType{};
    serialized.set_version(version_);
    serialize_deterministic(lock, *serialized.mutable_deterministic());
    auto local = proto::PaymentCode{};
    if (false == local_.get().Serialize(local)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to serialize local paymentcode")
            .Flush();

        return false;
    }
    *serialized.mutable_local() = local;
    auto remote = proto::PaymentCode{};
    if (false == remote_.get().Serialize(remote)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to serialize remote paymentcode")
            .Flush();

        return false;
    }
    *serialized.mutable_remote() = remote;

    {
        auto& dir = *serialized.mutable_incoming();
        dir.set_version(Bip47DirectionVersion);

        for (const auto& txid : incoming_notifications_) {
            dir.add_notification(
                static_cast<const char*>(txid->data()), txid->size());
        }

        for (const auto& [index, address] : data_.external_.map_) {
            *dir.add_address() = address->Serialize();
        }
    }

    {
        auto& dir = *serialized.mutable_outgoing();
        dir.set_version(Bip47DirectionVersion);

        for (const auto& txid : outgoing_notifications_) {
            dir.add_notification(
                static_cast<const char*>(txid->data()), txid->size());
        }

        for (const auto& [index, address] : data_.internal_.map_) {
            *dir.add_address() = address->Serialize();
        }
    }

    // TODO serialised.set_contact();
    const bool saved = api_.Storage().Store(parent_.NymID(), id_, serialized);

    if (false == saved) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to save PaymentCode account")
            .Flush();

        return false;
    }

    return saved;
}
}  // namespace opentxs::api::client::blockchain::implementation
