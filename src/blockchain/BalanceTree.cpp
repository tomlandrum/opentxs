// Copyright (c) 2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/Log.hpp"

#include "BalanceTree.hpp"
#include "HDChain.hpp"

#define OT_METHOD "opentxs::blockchain::implementation::BalanceTree::"

namespace opentxs
{
blockchain::BalanceTree* Factory::BalanceTree(
    const api::client::Blockchain& blockchain,
    const identifier::Nym& nymid,
    const proto::ContactItemType type,
    const std::set<OTIdentifier>& accountids)
{
    return new blockchain::implementation::BalanceTree(
        blockchain, nymid, type, accountids);
}
}  // namespace opentxs

namespace opentxs::blockchain::implementation
{
BalanceTree::BalanceTree(
    const api::client::Blockchain& blockchain,
    const identifier::Nym& nymid,
    const proto::ContactItemType type,
    const std::set<OTIdentifier>& accountids)
    : blockchain_(blockchain)
    , nymid_(nymid.str())
    , type_(type)
    , hdchain_ids_{}
    , imported_ids_{}
    , paymentcode_ids_{}
    , hdchains_{}
    , imported_{}
    , paymentcodes_{}
    , callback_()
{
    // TODO
    switch (BalanceTree::get_subtype(type)) {
        case blockchain::SubaccountType::HD: {
            for (const auto id : accountids) { hdchain_ids_.emplace_back(id); }
        } break;
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported blockchain type")
                .Flush();
        }
    }
}

bool BalanceTree::AddHDChain(const opentxs::blockchain::HDChain& hdchain) const
{
    // TODO
    return false;
}

bool BalanceTree::AssociateOutgoingTransaction(
    const proto::BlockchainTransaction& transaction) const
{
    // TODO
    return false;
}

std::size_t BalanceTree::CreatePaymentCodeChain(
    const std::string& localPaymentCode,
    const std::string& localRemoteCode) const
{
    // TODO
    return 0;
}

std::size_t BalanceTree::GetHDCount() const { return hdchains_.size(); }

std::size_t BalanceTree::GetImportedCount() const { return imported_.size(); }

std::size_t BalanceTree::GetPaymentCodeCount() const
{
    return paymentcodes_.size();
}

SubaccountType BalanceTree::get_subtype(proto::ContactItemType type)
{
    switch (type) {
        // TODO
        case proto::CITEMTYPE_BTC:
            return blockchain::SubaccountType::HD;
        default:
            return blockchain::SubaccountType::HD;
    }
}

const blockchain::HDChain& BalanceTree::HDChain(const std::size_t index) const
{
    if (index >= hdchain_ids_.size()) {
        throw std::out_of_range("Invalid index.");
    }

    const auto accountid = hdchain_ids_[index];
    auto hdchain = hdchains_.at(accountid);
    if (nullptr == hdchain.get()) {
        hdchain.reset(Factory::HDChain(blockchain_, *this, accountid));
    }
    return *hdchain;
}

const blockchain::Imported& BalanceTree::Imported(const std::size_t index) const
{
    if (index >= imported_ids_.size()) {
        throw std::out_of_range("Invalid index.");
    }

    const auto accountid = imported_ids_[index];
    auto imported = imported_.at(accountid);
    if (nullptr == imported.get()) {
        // TODO
        // imported.reset(Factory::Imported(blockchain_, *this, accountid));
    }
    return *imported;
}

const std::string& BalanceTree::NymID() const
{
    return nymid_;
}

const blockchain::PaymentCodeChain& BalanceTree::PaymentCodeChain(
    const std::size_t index) const
{
    if (index >= paymentcode_ids_.size()) {
        throw std::out_of_range("Invalid index.");
    }

    const auto accountid = paymentcode_ids_[index];
    auto paymentcode = paymentcodes_.at(accountid);
    if (nullptr == paymentcode.get()) {
        // TODO
        // paymentcode.reset(Factory::PaymentCodeChain(blockchain_, *this,
        // accountid));
    }
    return *paymentcode;
}

bool BalanceTree::SetNewSubaccountTypeCallback(
    SubaccountTypeAddedCallback&& callback) const
{
    callback_ = callback;

    return bool(callback_);
}
}  // namespace opentxs::blockchain::implementation
