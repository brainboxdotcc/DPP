/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * Copyright 2021 Craig Edwards and D++ contributors
 * (https://github.com/brainboxdotcc/DPP/graphs/contributors)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/
#pragma once

/**
 * Include actual kqueue
 */
#include <sys/event.h>
#include <sys/sysctl.h>

#if defined __NetBSD__ && __NetBSD_Version__ <= 999001400
	#define CAST_TYPE intptr_t
#else
	#define CAST_TYPE void*
#endif

#ifndef EV_ONESHOT

#include <cstdint>

/**
 * This is a facade for kqueue(), a freebsd-only event mechanism.
 * It is not documented here and only exists so that when editing the file on Linux
 * the linter does not go nuts. For actual documentation of kqueue,
 * see the relevant man pages.
 */

int kqueue();

int kevent(int kq, const struct kevent *changelist, int number_changes, struct kevent *event_list, std::size_t number_events, const struct timespec *timeout);

#define EV_SET(kev, ident, filter, flags, fflags, data, udata)

struct kevent {
	uintptr_t ident;	       /* identifier for this event */
	short filter;       /* filter for event */
	uint16_t flags;	       /* action flags for kqueue */
	uint32_t fflags;       /* filter flag value */
	intptr_t data;	       /* filter data value */
	void *udata;       /* opaque user data identifier */
};

#define EV_ADD		0x0001
#define EV_DELETE	0x0002
#define EV_ONESHOT	0x0010
#define EV_EOF		0x8000
#define EV_ERROR	0x4000
#define EVFILT_READ	(-1)
#define EVFILT_WRITE	(-2)

#endif
