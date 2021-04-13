// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_PROFILESUBSECTION_HPP
#define OPENTXS_UI_PROFILESUBSECTION_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <string>

#include "opentxs/Proto.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/ui/List.hpp"
#include "opentxs/ui/ListRow.hpp"

#ifdef SWIG
// clang-format off
%extend opentxs::ui::ProfileSubsection {
    int Type() const noexcept
    {
        return static_cast<int>($self->Type());
    }
}
%ignore opentxs::ui::ProfileSubsection::Type;
%ignore opentxs::ui::ProfileSubsection::Update;
%template(OTUIProfileSubsection) opentxs::SharedPimpl<opentxs::ui::ProfileSubsection>;
%rename(UIProfileSubsection) opentxs::ui::ProfileSubsection;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace ui
{
class ProfileSubsection;
}  // namespace ui

using OTUIProfileSubsection = SharedPimpl<ui::ProfileSubsection>;
}  // namespace opentxs

namespace opentxs
{
namespace ui
{
class ProfileSubsection : virtual public List, virtual public ListRow
{
public:
    OPENTXS_EXPORT virtual bool AddItem(
        const std::string& value,
        const bool primary,
        const bool active) const noexcept = 0;
    OPENTXS_EXPORT virtual bool Delete(
        const std::string& claimID) const noexcept = 0;
    OPENTXS_EXPORT virtual opentxs::SharedPimpl<opentxs::ui::ProfileItem>
    First() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string Name(
        const std::string& lang) const noexcept = 0;
    OPENTXS_EXPORT virtual opentxs::SharedPimpl<opentxs::ui::ProfileItem> Next()
        const noexcept = 0;
    OPENTXS_EXPORT virtual bool SetActive(
        const std::string& claimID,
        const bool active) const noexcept = 0;
    OPENTXS_EXPORT virtual bool SetPrimary(
        const std::string& claimID,
        const bool primary) const noexcept = 0;
    OPENTXS_EXPORT virtual bool SetValue(
        const std::string& claimID,
        const std::string& value) const noexcept = 0;
    OPENTXS_EXPORT virtual contact::ContactItemType Type() const noexcept = 0;

    OPENTXS_EXPORT ~ProfileSubsection() override = default;

protected:
    ProfileSubsection() noexcept = default;

private:
    ProfileSubsection(const ProfileSubsection&) = delete;
    ProfileSubsection(ProfileSubsection&&) = delete;
    ProfileSubsection& operator=(const ProfileSubsection&) = delete;
    ProfileSubsection& operator=(ProfileSubsection&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif
