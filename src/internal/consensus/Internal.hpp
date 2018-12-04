// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

#include "opentxs/consensus/ClientContext.hpp"
#include "opentxs/consensus/Context.hpp"
#include "opentxs/consensus/ServerContext.hpp"

namespace opentxs::internal
{
struct Context : virtual public opentxs::Context {
    virtual proto::Context GetContract(const Lock& lock) const = 0;
    virtual bool ValidateContext(const Lock& lock) const = 0;

    virtual std::mutex& GetLock() = 0;
    virtual bool UpdateSignature(const Lock& lock) = 0;

    virtual ~Context() = default;
};
struct ClientContext : virtual public opentxs::ClientContext,
                       virtual public internal::Context {

    virtual ~ClientContext() = default;
};
struct ServerContext : virtual public opentxs::ServerContext,
                       virtual public internal::Context {

    virtual ~ServerContext() = default;
};
}  // namespace opentxs::internal
