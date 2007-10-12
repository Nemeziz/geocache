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

#ifndef __GC_DEBUG_H__
#define __GC_DEBUG_H__

#include "gc_error.h"

#ifdef GC_DEBUG
#define gc_debug(x) x
#else
#define gc_debug(x)
#endif 

#define not_null(p) if (!(p)) {                         \
        gc_error("#p is NULL");                         \
        return -1;                                      \
    }

#define not_null_void(p) if (!(p)) {                    \
        gc_error("#p is NULL");                         \
    }

#endif


