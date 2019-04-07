// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

#include "internal/blockchain/Blockchain.hpp"

#include <opentxs/blockchain/BalanceTree.hpp>

#include <vector>
#include <set>

namespace opentxs::blockchain::implementation
{
class BalanceTree : virtual public blockchain::internal::BalanceTree,
                    public blockchain::BalanceTree
{
public:
    static SubaccountType get_subtype(proto::ContactItemType type);

    bool AddHDChain(const opentxs::blockchain::HDChain& hdchain) const override;

    std::size_t CreatePaymentCodeChain(
        const std::string& localPaymentCode,
        const std::string& localRemoteCode) const override;

    std::size_t GetHDCount() const override;
    std::size_t GetImportedCount() const override;
    std::size_t GetPaymentCodeCount() const override;

    const blockchain::HDChain& HDChain(const std::size_t index) const override;
    const blockchain::Imported& Imported(
        const std::size_t index) const override;
    const blockchain::PaymentCodeChain& PaymentCodeChain(
        const std::size_t index) const override;

    bool SetNewSubaccountTypeCallback(
        SubaccountTypeAddedCallback&& callback) const override;

    bool AssociateOutgoingTransaction(
        const proto::BlockchainTransaction& transaction) const override;

    ~BalanceTree() override = default;

protected:
    friend opentxs::Factory;

    BalanceTree(
        const api::client::Blockchain&,
        const identifier::Nym& nymid,
        const proto::ContactItemType type,
        const std::set<OTIdentifier>& accountids);

private:
    const api::client::Blockchain& blockchain_;
    const OTNymID nymid_;
    const proto::ContactItemType type_;
    std::vector<OTIdentifier> hdchain_ids_{};
    std::vector<OTIdentifier> imported_ids_{};
    std::vector<OTIdentifier> paymentcode_ids_{};
    std::map<OTIdentifier, std::shared_ptr<blockchain::HDChain>> hdchains_{};
    std::map<OTIdentifier, std::shared_ptr<blockchain::Imported>> imported_{};
    std::map<OTIdentifier, std::shared_ptr<blockchain::PaymentCodeChain>>
        paymentcodes_{};
    mutable SubaccountTypeAddedCallback callback_;

    BalanceTree() = delete;
    BalanceTree(const BalanceTree&) = delete;
    BalanceTree(BalanceTree&&) = delete;
    BalanceTree& operator=(const BalanceTree&) = delete;
    BalanceTree& operator=(BalanceTree&&) = delete;
};
}  // namespace opentxs::blockchain::implementation
