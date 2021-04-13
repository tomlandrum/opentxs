// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_CONTACTSECTION_HPP
#define OPENTXS_UI_CONTACTSECTION_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <string>

#include "opentxs/Proto.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/contact/Types.hpp"
#include "opentxs/ui/List.hpp"
#include "opentxs/ui/ListRow.hpp"

#ifdef SWIG
// clang-format off
%extend opentxs::ui::ContactSection {
    int Type() const
    {
        return static_cast<int>($self->Type());
    }
}
%ignore opentxs::ui::ContactSection::Type;
%ignore opentxs::ui::ContactSection::Update;
%template(OTUIContactSection) opentxs::SharedPimpl<opentxs::ui::ContactSection>;
%rename(UIContactSection) opentxs::ui::ContactSection;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace ui
{
class ContactSection;
class ContactSubsection;
}  // namespace ui

using OTUIContactSection = SharedPimpl<ui::ContactSection>;
}  // namespace opentxs

namespace opentxs
{
namespace ui
{
class ContactSection : virtual public List, virtual public ListRow
{
public:
    OPENTXS_EXPORT virtual std::string Name(
        const std::string& lang) const noexcept = 0;
    OPENTXS_EXPORT virtual opentxs::SharedPimpl<opentxs::ui::ContactSubsection>
    First() const noexcept = 0;
    OPENTXS_EXPORT virtual opentxs::SharedPimpl<opentxs::ui::ContactSubsection>
    Next() const noexcept = 0;
    OPENTXS_EXPORT virtual contact::ContactSectionName Type()
        const noexcept = 0;

    OPENTXS_EXPORT ~ContactSection() override = default;

protected:
    ContactSection() noexcept = default;

private:
    ContactSection(const ContactSection&) = delete;
    ContactSection(ContactSection&&) = delete;
    ContactSection& operator=(const ContactSection&) = delete;
    ContactSection& operator=(ContactSection&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif
