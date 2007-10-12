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

#ifndef __GC_CONN_H__
#define __GC_CONN_H__

#include <stddef.h>

struct gc_conn_item_t;
struct gc_conn_internal_t;
struct gc_db_t;

struct gc_conn_t {
    size_t size;
    struct gc_db_t *db;
    struct gc_conn_item_t *items;
    struct gc_conn_internal_t *internal;
};

int gc_conn_init(struct gc_conn_t **conn, size_t size);
int gc_conn_add(struct gc_conn_t *conn, int fd, unsigned int timeout);
size_t gc_conn_process(struct gc_conn_t *conn);
int gc_conn_load_key_file(struct gc_conn_t *conn, const char *filename);
int gc_conn_free(struct gc_conn_t *conn);

#endif


