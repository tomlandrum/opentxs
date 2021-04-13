// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/ui/BlockchainAccountListItem.cpp"

#pragma once

#if OT_QT
#include <QVariant>
#endif  // OT_QT
#include <string>

#include "1_Internal.hpp"
#include "internal/blockchain/Blockchain.hpp"
#include "internal/ui/UI.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/ui/AccountListItem.hpp"
#include "ui/base/Row.hpp"

namespace opentxs
{
namespace api
{
namespace client
{
namespace internal
{
struct Manager;
}  // namespace internal
}  // namespace client

class Core;
}  // namespace api

namespace identifier
{
class Server;
class UnitDefinition;
}  // namespace identifier

namespace network
{
namespace zeromq
{
namespace socket
{
class Publish;
}  // namespace socket
}  // namespace zeromq
}  // namespace network

namespace ui
{
class BlockchainAccountListItem;
}  // namespace ui
}  // namespace opentxs

namespace opentxs::ui::implementation
{
using AccountListItemRow =
    Row<AccountListRowInternal, AccountListInternalInterface, AccountListRowID>;

class BlockchainAccountListItem final : public AccountListItemRow
{
public:
    auto AccountID() const noexcept -> std::string final
    {
        return row_id_->str();
    }
    auto Balance() const noexcept -> Amount final;
    auto ContractID() const noexcept -> std::string final { return contract_; }
    auto DisplayBalance() const noexcept -> std::string final
    {
        return blockchain::internal::Format(chain_, balance_);
    }
    auto DisplayUnit() const noexcept -> std::string final
    {
        return blockchain::internal::Ticker(chain_);
    }
    auto Name() const noexcept -> std::string final;
    auto NotaryID() const noexcept -> std::string final { return notary_; }
    auto NotaryName() const noexcept -> std::string final
    {
        return blockchain::DisplayString(chain_);
    }
    auto reindex(const AccountListSortKey& key, CustomData& custom) noexcept
        -> bool final;
    auto Type() const noexcept -> AccountType final { return type_; }
    auto Unit() const noexcept -> contact::ContactItemType final
    {
        return unit_;
    }

#if OT_QT
    auto qt_data(const int column, const int role) const noexcept
        -> QVariant final;
#endif

    BlockchainAccountListItem(
        const AccountListInternalInterface& parent,
        const api::client::internal::Manager& api,
        const AccountListRowID& rowID,
        const AccountListSortKey& sortKey,
        CustomData& custom) noexcept;

    ~BlockchainAccountListItem() final = default;

private:
    const AccountType type_;
    const contact::ContactItemType unit_;
    const blockchain::Type chain_;
    const std::string contract_;
    const std::string notary_;
    Amount balance_;
    std::string name_;

    BlockchainAccountListItem() = delete;
    BlockchainAccountListItem(const BlockchainAccountListItem&) = delete;
    BlockchainAccountListItem(BlockchainAccountListItem&&) = delete;
    auto operator=(const BlockchainAccountListItem&)
        -> BlockchainAccountListItem& = delete;
    auto operator=(BlockchainAccountListItem&&)
        -> BlockchainAccountListItem& = delete;
};
}  // namespace opentxs::ui::implementation
