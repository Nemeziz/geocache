/* Copyright (C) 2007 Yung-chung Lin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef __GC_LOG_H__
#define __GC_LOG_H__

#include <stdio.h>
#include <syslog.h>

#include "gc_error.h"

#define gc_logl(l, ...) do {                    \
        if (g_is_daemon) {                      \
            syslog(l, __VA_ARGS__);             \
        }                                       \
        else {                                  \
            fprintf(stderr, __VA_ARGS__);       \
            fprintf(stderr, "\n");              \
        }                                       \
    } while (0)

#define gc_log(...) gc_logl(LOG_INFO, __VA_ARGS__)

#define gc_loge(...) gc_logl(LOG_ERR, __VA_ARGS__)

#endif
