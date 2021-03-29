// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_NYMIDSOURCE_HPP
#define OPENTXS_CORE_NYMIDSOURCE_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <memory>

#include "opentxs/Proto.hpp"
#include "opentxs/identity/Types.hpp"

namespace opentxs
{
namespace identity
{
namespace credential
{
class Primary;
}  // namespace credential
}  // namespace identity

namespace proto
{
class NymIDSource;
}  // namespace proto
}  // namespace opentxs

namespace opentxs
{
namespace identity
{
class Source
{
public:
    OPENTXS_EXPORT virtual OTString asString() const noexcept = 0;
    OPENTXS_EXPORT virtual OTString Description() const noexcept = 0;
    OPENTXS_EXPORT virtual identity::SourceType Type() const noexcept = 0;
    OPENTXS_EXPORT virtual OTNymID NymID() const noexcept = 0;
    OPENTXS_EXPORT virtual std::shared_ptr<proto::NymIDSource> Serialize()
        const noexcept = 0;
    OPENTXS_EXPORT virtual bool Verify(
        const proto::Credential& master,
        const proto::Signature& sourceSignature) const noexcept = 0;
    OPENTXS_EXPORT virtual bool Sign(
        const identity::credential::Primary& credential,
        proto::Signature& sig,
        const PasswordPrompt& reason) const noexcept = 0;

    OPENTXS_EXPORT virtual ~Source() = default;

protected:
    Source() = default;

private:
    Source(const Source&) = delete;
    Source(Source&&) = delete;
    Source& operator=(const Source&);
    Source& operator=(Source&&);
};
}  // namespace identity
}  // namespace opentxs
#endif
