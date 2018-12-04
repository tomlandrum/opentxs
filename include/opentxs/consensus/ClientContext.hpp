// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CONSENSUS_CLIENTCONTEXT_HPP
#define OPENTXS_CONSENSUS_CLIENTCONTEXT_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/consensus/Context.hpp"

namespace opentxs
{
class ClientContext : virtual public Context
{
public:
    EXPORT virtual bool hasOpenTransactions() const = 0;
    using Context::IssuedNumbers;
    EXPORT virtual std::size_t IssuedNumbers(
        const TransactionNumbers& exclude) const = 0;
    EXPORT virtual std::size_t OpenCronItems() const = 0;
    EXPORT virtual bool Verify(
        const TransactionStatement& statement,
        const TransactionNumbers& excluded,
        const TransactionNumbers& included) const = 0;
    EXPORT virtual bool VerifyCronItem(
        const TransactionNumber number) const = 0;
    using Context::VerifyIssuedNumber;
    EXPORT virtual bool VerifyIssuedNumber(
        const TransactionNumber& number,
        const TransactionNumbers& exclude) const = 0;

    EXPORT virtual bool AcceptIssuedNumbers(TransactionNumbers& newNumbers) = 0;
    EXPORT virtual void FinishAcknowledgements(const RequestNumbers& req) = 0;
    EXPORT virtual bool IssueNumber(const TransactionNumber& number) = 0;

    virtual ~ClientContext() override = default;

protected:
    ClientContext() = default;

private:
    ClientContext(const ClientContext&) = delete;
    ClientContext(ClientContext&&) = delete;
    ClientContext& operator=(const ClientContext&) = delete;
    ClientContext& operator=(ClientContext&&) = delete;
};
}  // namespace opentxs
#endif
