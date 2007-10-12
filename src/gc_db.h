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

#ifndef __GC_DB_H__
#define __GC_DB_H__

struct gc_db_t;

struct gc_db_query_t {
    int code;
    char accuracy;
    double latitude;
    double longitude;
};

int gc_db_init(struct gc_db_t **db);
int gc_db_load(struct gc_db_t *db, const char *filename);
int gc_db_get(struct gc_db_t *db, const char *location,
              const struct gc_db_query_t *query);
int gc_db_put(struct gc_db_t *db, const char *location,
              const struct gc_db_query_t *query);
int gc_db_sync(struct gc_db_t *db);
int gc_db_free(struct gc_db_t *db);

#endif


