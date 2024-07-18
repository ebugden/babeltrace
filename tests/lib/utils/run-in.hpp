/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2020-2023 EfficiOS, inc.
 */

#ifndef BABELTRACE_TESTS_LIB_UTILS_RUN_IN_HPP
#define BABELTRACE_TESTS_LIB_UTILS_RUN_IN_HPP

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2/self-component-class.hpp"
#include "cpp-common/bt2/self-component-port.hpp"
#include "cpp-common/bt2/self-message-iterator.hpp"

/*
 * Base class from which to inherit to call runIn().
 *
 * Override any of the on*() methods to get your statements executed in
 * a specific context.
 */
class RunIn
{
public:
    virtual ~RunIn() = default;

    /*
     * Called when querying the component class `self`.
     */
    virtual void onQuery(bt2::SelfComponentClass self);

    /*
     * Called when initializing the component `self`.
     */
    virtual void onCompInit(bt2::SelfComponent self);

    /*
     * Called when initializing the message iterator `self`.
     */
    virtual void onMsgIterInit(bt2::SelfMessageIterator self);

    /*
     * Called within the "next" method of `self` to return the messages
     * `msgs`.
     */
    virtual void onMsgIterNext(bt2::SelfMessageIterator self, bt2::ConstMessageArray& msgs);
};

/*
 * Runs a simple graph (one source and one sink component), calling the
 * `on*()` methods of `runIn` along the way.
 *
 * Use `graphMipVersion` as the graph's MIP version.
 */
void runIn(RunIn& runIn, uint64_t graphMipVersion);

#endif /* BABELTRACE_TESTS_LIB_UTILS_RUN_IN_HPP */
