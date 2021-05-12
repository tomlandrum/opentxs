// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_SIGNABLE_HPP
#define OPENTXS_CORE_CONTRACT_SIGNABLE_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <string>

#include "opentxs/Types.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"

namespace opentxs
{
namespace proto
{
class Signature;
}  // namespace proto
}  // namespace opentxs

namespace opentxs
{
namespace contract
{
class OPENTXS_EXPORT Signable
{
public:
    using Signature = std::shared_ptr<proto::Signature>;

    virtual std::string Alias() const = 0;
    virtual OTIdentifier ID() const = 0;
    virtual std::string Name() const = 0;
    virtual Nym_p Nym() const = 0;
    virtual const std::string& Terms() const = 0;
    virtual OTData Serialize() const = 0;
    virtual bool Validate() const = 0;
    virtual VersionNumber Version() const = 0;

    virtual void SetAlias(const std::string& alias) = 0;

    virtual ~Signable() = default;

protected:
    Signable() noexcept = default;

private:
#ifdef _WIN32
public:
#endif
    virtual Signable* clone() const noexcept = 0;
#ifdef _WIN32
private:
#endif

    Signable(const Signable&) = delete;
    Signable(Signable&&) = delete;
    Signable& operator=(const Signable&) = delete;
    Signable& operator=(Signable&&) = delete;
};
}  // namespace contract
}  // namespace opentxs
#endif
