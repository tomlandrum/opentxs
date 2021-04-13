// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_UNITLISTITEM_HPP
#define OPENTXS_UI_UNITLISTITEM_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <string>

#include "ListRow.hpp"
#include "opentxs/SharedPimpl.hpp"

#ifdef SWIG
// clang-format off
%extend opentxs::ui::UnitListItem {
    int Unit() const
    {
        return static_cast<int>($self->Unit());
    }
}
%ignore opentxs::ui::UnitListItem::Unit;
%template(OTUIUnitListItem) opentxs::SharedPimpl<opentxs::ui::UnitListItem>;
%rename(UIUnitListItem) opentxs::ui::UnitListItem;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace ui
{
class UnitListItem;
}  // namespace ui

using OTUIUnitListItem = SharedPimpl<ui::UnitListItem>;
}  // namespace opentxs

namespace opentxs
{
namespace ui
{
class UnitListItem : virtual public ListRow
{
public:
    OPENTXS_EXPORT virtual std::string Name() const noexcept = 0;
    OPENTXS_EXPORT virtual contact::ContactItemType Unit() const noexcept = 0;

    ~UnitListItem() override = default;

protected:
    UnitListItem() noexcept = default;

private:
    UnitListItem(const UnitListItem&) = delete;
    UnitListItem(UnitListItem&&) = delete;
    UnitListItem& operator=(const UnitListItem&) = delete;
    UnitListItem& operator=(UnitListItem&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif
