// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

#include "opentxs/api/storage/Plugin.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"

#include <atomic>
#include <string>

namespace opentxs
{
class StorageConfig;

namespace api
{
namespace storage
{
class Storage;
}  // namespace storage
}  // namespace api

class Plugin : virtual public opentxs::api::storage::Plugin
{
public:
    bool EmptyBucket(const bool bucket) const override = 0;

    bool Load(const std::string& key, const bool checking, std::string& value)
        const override;
    bool LoadFromBucket(
        const std::string& key,
        std::string& value,
        const bool bucket) const override = 0;
    bool Store(
        const bool isTransaction,
        const std::string& key,
        const std::string& value,
        const bool bucket) const override;
    void Store(
        const bool isTransaction,
        const std::string& key,
        const std::string& value,
        const bool bucket,
        std::promise<bool>& promise) const override;
    bool Store(
        const bool isTransaction,
        const std::string& value,
        std::string& key) const override;

    bool Migrate(
        const std::string& key,
        const opentxs::api::storage::Driver& to) const override;

    std::string LoadRoot() const override = 0;
    bool StoreRoot(const bool commit, const std::string& hash) const override =
        0;

    virtual void Cleanup() = 0;

    virtual ~Plugin() = default;

protected:
    const StorageConfig& config_;
    const Random& random_;

    Plugin(
        const api::storage::Storage& storage,
        const StorageConfig& config,
        const Digest& hash,
        const Random& random,
        const Flag& bucket);
    Plugin() = delete;

    virtual void store(
        const bool isTransaction,
        const std::string& key,
        const std::string& value,
        const bool bucket,
        std::promise<bool>* promise) const = 0;

private:
    const api::storage::Storage& storage_;
    const Digest& digest_;
    const Flag& current_bucket_;

    Plugin(const Plugin&) = delete;
    Plugin(Plugin&&) = delete;
    Plugin& operator=(const Plugin&) = delete;
    Plugin& operator=(Plugin&&) = delete;
};

template <class T>
bool opentxs::api::storage::Driver::LoadProto(
    const std::string& hash,
    std::shared_ptr<T>& serialized,
    const bool checking) const
{
    std::string raw;
    const bool loaded = Load(hash, checking, raw);
    bool valid = false;

    if (loaded) {
        serialized.reset(new T);
        serialized->ParseFromArray(raw.data(), raw.size());
        valid = proto::Validate<T>(*serialized, VERBOSE);
    }

    if (!valid) {
        if (loaded) {
            LogOutput(": Specified object was located but could not be "
                      "validated.")
                .Flush();
            LogOutput(": Hash: ")(hash).Flush();
            LogOutput(": Size: ")(raw.size()).Flush();
        } else {

            LogDetail("Specified object is missing.").Flush();
            LogDetail("Hash: ")(hash).Flush();
            LogDetail("Size: ")(raw.size()).Flush();
        }
    }

    OT_ASSERT(valid);

    return valid;
}

template <class T>
bool opentxs::api::storage::Driver::StoreProto(
    const T& data,
    std::string& key,
    std::string& plaintext) const
{
    if (!proto::Validate<T>(data, VERBOSE)) { return false; }

    plaintext = proto::ProtoAsString<T>(data);

    return Store(true, plaintext, key);
}

template <class T>
bool opentxs::api::storage::Driver::StoreProto(const T& data, std::string& key)
    const
{
    std::string notUsed;

    return StoreProto<T>(data, key, notUsed);
}

template <class T>
bool opentxs::api::storage::Driver::StoreProto(const T& data) const
{
    std::string notUsed;

    return StoreProto<T>(data, notUsed);
}
}  // namespace opentxs
