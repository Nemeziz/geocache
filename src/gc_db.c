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

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <db.h>
#include <errno.h>

#include "gc_debug.h"
#include "gc_error.h"
#include "gc_log.h"
#include "gc_db.h"
#include "gc_util.h"

struct gc_db_t {
    DB * bdb;
};

extern int g_is_daemon;

int gc_db_init(struct gc_db_t **db) {
    not_null(db);

    *db = malloc(sizeof(struct gc_db_t));
    if (!*db) {
        gc_loge("Cannot allocate memory for database");
        return -1;
    }
    
    if (db_create(&((*db)->bdb), NULL, 0) != 0) {
        safefree(*db);
        return -1;
    }

    return 0;
}

int gc_db_load(struct gc_db_t *db, const char *filename) {
    not_null(db);
    not_null(filename);

    int ret = 0;
    char pathname[512];

    gc_get_path_of(filename, pathname, 512);
    
    if (mkdir(pathname, 0644) != 0) {
        if (errno != EEXIST) {
            gc_loge("Cannot create directory '%s': %m", pathname);
        }
    }
    
    ret = db->bdb->open(db->bdb, NULL, filename, NULL, DB_BTREE, DB_CREATE, 0);
    if (ret != 0) {
        gc_loge("Cannot open database file: %s", db_strerror(ret));
        return -1;
    }

    return 0;
}

int gc_db_get(struct gc_db_t *db, const char *location,
              const struct gc_db_query_t *query) {
    not_null(db);
    not_null(location);
    not_null(query);

    int ret = 0;
    DBT key;
    DBT data;

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    key.data = (void*) location;
    key.size = strlen(location);

    data.data = (void*) query;
    data.size = sizeof(struct gc_db_query_t);

    ret = db->bdb->get(db->bdb, NULL, &key, &data, 0);
    memcpy((void*) query, data.data, sizeof(struct gc_db_query_t));
    
    if (ret != 0) {
        if (ret != DB_NOTFOUND) {
            gc_loge("Cannot get data from database: %s", db_strerror(ret));
        }
        return -1;
    }

    return 0;
}

int gc_db_put(struct gc_db_t *db, const char *location,
              const struct gc_db_query_t *query) {
    not_null(db);
    not_null(location);
    not_null(query);

    int ret = 0;
    DBT key;
    DBT data;

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    key.data = (void*) location;
    key.size = strlen(location);

    data.data = (void*) query;
    data.size = sizeof(struct gc_db_query_t);

    ret = db->bdb->put(db->bdb, NULL, &key, &data, DB_NOOVERWRITE);
    if (ret != 0) {
        gc_loge("Cannot put data into database: %s", db_strerror(ret));
        return -1;
    }
    return 0;
}

int gc_db_sync(struct gc_db_t *db) {
    int ret = db->bdb->sync(db->bdb, 0);
    if (ret != 0) {
        gc_loge("Cannot sync database: %s", db_strerror(ret));
        return -1;
    }
    return 0;
}

int gc_db_free(struct gc_db_t *db) {
    not_null(db);

    if (db != NULL && db->bdb != NULL) {
        if (db->bdb->close(db->bdb, 0) != 0) {
            return -1;
        }
    }
    safefree(db);
    return 0;
}
