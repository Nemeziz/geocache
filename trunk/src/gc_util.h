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

#ifndef __GC_UTIL_H__
#define __GC_UTIL_H__

#include <stddef.h>
#include <netinet/in.h>

#define GC_MIN(a, b) ((a) < (b) ? (a) : (b))

#define safefree(p) if (p) {     \
        free(p);                 \
        p = NULL;                \
    }

size_t gc_chomp(char *buf, size_t buf_size);
int gc_socket_connect(in_addr_t host, int port);
int gc_set_nonblock(int fd);
size_t gc_get_path_of(const char *filename, char *buf, size_t buf_size);

#endif


