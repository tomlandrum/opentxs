// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_ACTIVITYTHREAD_HPP
#define OPENTXS_UI_ACTIVITYTHREAD_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <string>

#include "opentxs/Proto.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/ui/List.hpp"

#ifdef SWIG
// clang-format off
%extend opentxs::ui::ActivityThread {
    std::string PaymentCode(const int currency) const
    {
        return $self->PaymentCode(
            static_cast<opentxs::contact::ContactItemType>(currency));
    }
}
%ignore opentxs::ui::ActivityThread::PaymentCode;
%rename(UIActivityThread) opentxs::ui::ActivityThread;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace ui
{
class ActivityThread;
class ActivityThreadItem;
}  // namespace ui
}  // namespace opentxs

namespace opentxs
{
namespace ui
{
class ActivityThread : virtual public List
{
public:
    OPENTXS_EXPORT virtual std::string DisplayName() const noexcept = 0;
    OPENTXS_EXPORT virtual opentxs::SharedPimpl<opentxs::ui::ActivityThreadItem>
    First() const noexcept = 0;
    OPENTXS_EXPORT virtual opentxs::SharedPimpl<opentxs::ui::ActivityThreadItem>
    Next() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string GetDraft() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string Participants() const noexcept = 0;
    OPENTXS_EXPORT virtual bool Pay(
        const std::string& amount,
        const Identifier& sourceAccount,
        const std::string& memo = "",
        const PaymentType type = PaymentType::Cheque) const noexcept = 0;
    OPENTXS_EXPORT virtual bool Pay(
        const Amount amount,
        const Identifier& sourceAccount,
        const std::string& memo = "",
        const PaymentType type = PaymentType::Cheque) const noexcept = 0;
    OPENTXS_EXPORT virtual std::string PaymentCode(
        const contact::ContactItemType currency) const noexcept = 0;
    OPENTXS_EXPORT virtual bool SendDraft() const noexcept = 0;
    OPENTXS_EXPORT virtual bool SetDraft(
        const std::string& draft) const noexcept = 0;
    OPENTXS_EXPORT virtual std::string ThreadID() const noexcept = 0;

    OPENTXS_EXPORT ~ActivityThread() override = default;

protected:
    ActivityThread() noexcept = default;

private:
    ActivityThread(const ActivityThread&) = delete;
    ActivityThread(ActivityThread&&) = delete;
    ActivityThread& operator=(const ActivityThread&) = delete;
    ActivityThread& operator=(ActivityThread&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif
