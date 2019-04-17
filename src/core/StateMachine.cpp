// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "StateMachine.hpp"

namespace opentxs::internal
{
StateMachine::StateMachine(const Callback callback) noexcept
    : decision_lock_()
    , cb_(callback)
    , clean_(false)
    , shutdown_(false)
    , running_(false)
    , handle_()
    , stopping_()
    , stopping_future_(stopping_.get_future())
    , waiting_()
    , waiting_future_()
{
}

void StateMachine::clean(const Lock& lock) const noexcept
{
    stopping_.set_value();
    clean_.store(true);
}

void StateMachine::execute() const noexcept
{
    bool again{true};

    while (again) {
        again = cb_();

        if (shutdown_.load()) { break; }
    }

    Lock lock(decision_lock_);
    running_.store(false);
    waiting_.set_value();

    if (shutdown_.load()) { clean(lock); }
}

StateMachine::WaitFuture StateMachine::make_wait_promise(
    const Lock& lock,
    const bool set) const noexcept
{
    waiting_ = {};
    waiting_future_ = waiting_.get_future();

    if (set) { waiting_.set_value(); }

    return waiting_future_;
}

StateMachine::StopFuture StateMachine::Stop() const noexcept
{
    Lock lock(decision_lock_);

    if (false == clean_.load()) {
        shutdown_.store(true);

        if (false == running_.load()) { clean(lock); }
    }

    return stopping_future_;
}

bool StateMachine::trigger(const Lock& lock) const noexcept
{
    if (shutdown_.load()) { return false; }

    const auto running = running_.exchange(true);

    if (running) { return true; }

    if (handle_.joinable()) { handle_.join(); }

    make_wait_promise(lock, false);
    handle_ = std::thread(&StateMachine::execute, this);

    return true;
}

bool StateMachine::Trigger() const noexcept
{
    Lock lock(decision_lock_);

    return trigger(lock);
}

StateMachine::WaitFuture StateMachine::Wait() const noexcept
{
    Lock lock(decision_lock_);

    if (false == running_.load()) { return make_wait_promise(lock, true); }

    return waiting_future_;
}

StateMachine::~StateMachine()
{
    if (false == clean_.load()) { Stop(); }

    if (handle_.joinable()) { handle_.join(); }
}
}  // namespace opentxs::internal