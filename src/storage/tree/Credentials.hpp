// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>
#include <mutex>
#include <string>

#include "opentxs/Proto.hpp"
#include "opentxs/api/Editor.hpp"
#include "opentxs/identity/Types.hpp"
#include "opentxs/protobuf/Enums.pb.h"
#include "opentxs/protobuf/StorageCredentials.pb.h"
#include "storage/tree/Node.hpp"

namespace opentxs
{
namespace api
{
namespace storage
{
class Driver;
}  // namespace storage
}  // namespace api

namespace proto
{
class Credential;
}  // namespace proto

namespace storage
{
class Tree;
}  // namespace storage
}  // namespace opentxs

namespace opentxs
{
namespace storage
{
class Credentials final : public Node
{
private:
    using KeyModeMap = std::map<identity::KeyMode, proto::KeyMode>;
    using KeyModeReverseMap = std::map<proto::KeyMode, identity::KeyMode>;

    static const KeyModeMap keymode_map_;
    static const KeyModeReverseMap keymode_reverse_map_;

    friend Tree;

    auto check_existing(const bool incoming, Metadata& metadata) const -> bool;
    void init(const std::string& hash) final;
    auto save(const std::unique_lock<std::mutex>& lock) const -> bool final;
    auto serialize() const -> proto::StorageCredentials;

    Credentials(
        const opentxs::api::storage::Driver& storage,
        const std::string& hash);
    Credentials() = delete;
    Credentials(const Credentials&) = delete;
    Credentials(Credentials&&) = delete;
    auto operator=(const Credentials&) -> Credentials = delete;
    auto operator=(Credentials&&) -> Credentials = delete;

public:
    auto Alias(const std::string& id) const -> std::string;
    auto Load(
        const std::string& id,
        std::shared_ptr<proto::Credential>& output,
        const bool checking) const -> bool;

    auto Delete(const std::string& id) -> bool;
    auto SetAlias(const std::string& id, const std::string& alias) -> bool;
    auto Store(const proto::Credential& data, const std::string& alias) -> bool;

    ~Credentials() final = default;
};
}  // namespace storage
}  // namespace opentxs
