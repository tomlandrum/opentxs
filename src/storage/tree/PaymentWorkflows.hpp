// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Editor.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/protobuf/PaymentWorkflowEnums.pb.h"
#include "opentxs/protobuf/StoragePaymentWorkflows.pb.h"
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
class PaymentWorkflow;
}  // namespace proto

namespace storage
{
class Nym;
}  // namespace storage
}  // namespace opentxs

namespace opentxs::storage
{
class PaymentWorkflows final : public Node
{
public:
    using State = std::pair<
        api::client::PaymentWorkflowType,
        api::client::PaymentWorkflowState>;
    using Workflows = std::set<std::string>;

    auto GetState(const std::string& workflowID) const -> State;
    auto ListByAccount(const std::string& accountID) const -> Workflows;
    auto ListByState(
        api::client::PaymentWorkflowType type,
        api::client::PaymentWorkflowState state) const -> Workflows;
    auto ListByUnit(const std::string& unitID) const -> Workflows;
    auto Load(
        const std::string& id,
        std::shared_ptr<proto::PaymentWorkflow>& output,
        const bool checking) const -> bool;
    auto LookupBySource(const std::string& sourceID) const -> std::string;

    auto Delete(const std::string& id) -> bool;
    auto Store(const proto::PaymentWorkflow& data, std::string& plaintext)
        -> bool;

    ~PaymentWorkflows() final = default;

private:
    friend Nym;

    Workflows archived_;
    std::map<std::string, std::string> item_workflow_map_;
    std::map<std::string, Workflows> account_workflow_map_;
    std::map<std::string, Workflows> unit_workflow_map_;
    std::map<std::string, State> workflow_state_map_;
    std::map<api::client::PaymentWorkflowType, Workflows> type_workflow_map_;
    std::map<State, Workflows> state_workflow_map_;

    auto save(const Lock& lock) const -> bool final;
    auto serialize() const -> proto::StoragePaymentWorkflows;

    void add_state_index(
        const Lock& lock,
        const std::string& workflowID,
        api::client::PaymentWorkflowType type,
        api::client::PaymentWorkflowState state);
    void delete_by_value(const std::string& value);
    void init(const std::string& hash) final;
    void reindex(
        const Lock& lock,
        const std::string& workflowID,
        const api::client::PaymentWorkflowType type,
        const api::client::PaymentWorkflowState newState,
        api::client::PaymentWorkflowState& state);

    PaymentWorkflows(
        const opentxs::api::storage::Driver& storage,
        const std::string& key);
    PaymentWorkflows() = delete;
    PaymentWorkflows(const PaymentWorkflows&) = delete;
    PaymentWorkflows(PaymentWorkflows&&) = delete;
    auto operator=(const PaymentWorkflows&) -> PaymentWorkflows = delete;
    auto operator=(PaymentWorkflows&&) -> PaymentWorkflows = delete;
};
}  // namespace opentxs::storage
