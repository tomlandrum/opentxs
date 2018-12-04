// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

#include "opentxs/api/Native.hpp"

namespace
{
/** Callbacks in this form allow OpenSSL to query opentxs to get key encryption
 *  and decryption passwords*/
extern "C" {
typedef std::int32_t INTERNAL_PASSWORD_CALLBACK(
    char* buf,
    std::int32_t size,
    std::int32_t rwflag,
    void* userdata);
}
}  // namespace

namespace opentxs::internal
{
}  // namespace opentxs::internal

namespace opentxs::api::internal
{
struct Native : virtual public api::Native {
    virtual INTERNAL_PASSWORD_CALLBACK* GetInternalPasswordCallback() const = 0;
    virtual OTCaller& GetPasswordCaller() const = 0;
    virtual void Init() = 0;
    virtual void shutdown() = 0;

    virtual ~Native() = default;
};

struct Log {
    virtual ~Log() = default;
};
}  // namespace opentxs::api::internal
