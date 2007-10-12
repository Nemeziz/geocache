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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gc_error.h"
#include "gc_log.h"
#include "gc_conn.h"
#include "gc_db.h"
#include "gc_debug.h"
#include "gc_util.h"

#define CONN_BUF_SIZE         256

#define CONN_ST_NULL          0
#define CONN_ST_INIT          1
#define CONN_ST_GOT_REQUEST   2
#define CONN_ST_REMOTE_OPENED 3
#define CONN_ST_FORWARDED     4
#define CONN_ST_REMOTE_CLOSED 5

#define GEOCODING_OUTPUT_FMT  "%d,%c,%lf,%lf\n"

#define GMAP_SERVER_HOSTNAME  "maps.google.com"
#define GMAP_KEY_SIZE         128
#define GMAP_SERVER_MAX_COUNT 5

struct gc_conn_item_t {
    int client_fd;
    int remote_fd;              /* fd to remote geocoding service */
    char status;
    time_t exptime;              /* expiration time */
    struct gc_db_query_t result; /* Geocoding result */
    size_t rd_buf_len;
    size_t wr_buf_pos;
    size_t wr_buf_len;
    char rd_buf[CONN_BUF_SIZE];
    char wr_buf[CONN_BUF_SIZE];
    char location[CONN_BUF_SIZE];
};

struct gc_conn_internal_t {
    size_t gmap_server_count;
    struct in_addr gmap_servers[GMAP_SERVER_MAX_COUNT];
    char gmap_key[GMAP_KEY_SIZE];
};

extern int h_errno;
extern int g_is_daemon;

static void _reset_item(struct gc_conn_item_t *item) {
    not_null_void(item);

    item->status = CONN_ST_NULL;

    if (item->client_fd >= 0 && close(item->client_fd) != 0) {
        gc_loge("Cannot close client fd: %m");
    }
    item->client_fd = -1;
    
    if (item->remote_fd >= 0 && close(item->remote_fd) != 0) {
        gc_loge("Cannot close remote fd: %m");
    }
    item->remote_fd = -1;
    
    item->rd_buf_len = 0;
    item->wr_buf_pos = 0;
    item->wr_buf_len = 0;

    item->location[0] = '\0';
}

static int _check_request(const char *buf, size_t buf_size) {
    static const char safe_char[]
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
        "0123456789\\-_.!~*'()%";
    const size_t safe_char_len = strlen(safe_char);
    int is_safe = 0;
    register size_t i = 0;
    register size_t j = 0;

    for (i = 0; i < buf_size; ++i) {
        is_safe = 0;
        for (j = 0; j < safe_char_len; ++j) {
            if (safe_char[j] == buf[i]) {
                is_safe = 1;
                break;
            }
        }
        if (!is_safe) {
            gc_loge("Non-safe character %d is received", (int) buf[i]);
            return 0;
        }
    }
    return 1;
}

static void _read_request(struct gc_conn_t *conn, struct gc_conn_item_t *item) {
    not_null_void(conn);
    not_null_void(item);

    ssize_t ret = read(item->client_fd, item->rd_buf + item->rd_buf_len,
                       CONN_BUF_SIZE - item->rd_buf_len);
    if (ret > 0) {
        item->rd_buf_len += ret;
        /* Overflowed buffer are deemed as attacks. */
        if (item->rd_buf_len >= CONN_BUF_SIZE) {
            _reset_item(item);
            return;
        }
        item->rd_buf[item->rd_buf_len - 1] = '\0';
    }
    else if (ret == 0) {
        if (!item->rd_buf_len) {
            _reset_item(item);
            return;
        }
        item->status = CONN_ST_GOT_REQUEST;
    }
    else if (ret < 0 && errno != EINPROGRESS) {
        _reset_item(item);
        return;
    }

    /* Check if we have got a newline. We just need the first line. */
    if (item->status != CONN_ST_GOT_REQUEST) {
        if (gc_chomp(item->rd_buf, item->rd_buf_len) < item->rd_buf_len) {
            item->rd_buf_len = strlen(item->rd_buf);
            item->status = CONN_ST_GOT_REQUEST;
        }
    }
    
    if (item->rd_buf_len && item->status == CONN_ST_GOT_REQUEST) {
        if (!_check_request(item->rd_buf, item->rd_buf_len)) {
            _reset_item(item);
            return;
        }

        gc_log("Query: [%s]", item->rd_buf);

        if (gc_db_get(conn->db, item->rd_buf, &(item->result)) == 0) {
            /* Data found in local database */
            snprintf(item->wr_buf, CONN_BUF_SIZE, GEOCODING_OUTPUT_FMT,
                     item->result.code, item->result.accuracy,
                     item->result.latitude, item->result.longitude);
            item->wr_buf_len = strlen(item->wr_buf);
            item->status = CONN_ST_REMOTE_CLOSED;
        }
        else {
            /* Prepare the request to geocoding service */
            strncpy(item->location, item->rd_buf, item->rd_buf_len);
            item->location[item->rd_buf_len] = '\0';

            snprintf(item->wr_buf, CONN_BUF_SIZE,
                     "GET /maps/geo?q=%s&output=csv&key=%s\n",
                     item->rd_buf, conn->internal->gmap_key);
            item->wr_buf_len = strlen(item->wr_buf);
        }
    }
}

static void _open_remote(struct gc_conn_t *conn, struct gc_conn_item_t *item) {
    not_null_void(conn);
    not_null_void(item);

    int i = rand() % conn->internal->gmap_server_count;
    item->remote_fd
        = gc_socket_connect(conn->internal->gmap_servers[i].s_addr, 80);
    if (item->remote_fd < 0) {
        _reset_item(item);
        return;
    }
    item->status = CONN_ST_REMOTE_OPENED;
}

static void _write_remote(struct gc_conn_t *conn, struct gc_conn_item_t *item) {
    not_null_void(conn);
    not_null_void(item);

    ssize_t ret =  write(item->remote_fd, item->wr_buf + item->wr_buf_pos,
                         item->wr_buf_len - item->wr_buf_pos);
    if (ret > 0) {
        item->wr_buf_pos += ret;
    }
    else if (ret == 0) {
        item->rd_buf_len = 0;
        item->status = CONN_ST_FORWARDED;
    }
    else if (ret < 0 && errno != EINPROGRESS) {
        gc_loge("Cannot write request to remote: %m");
        _reset_item(item);
    }
}

static void _read_remote(struct gc_conn_t *conn, struct gc_conn_item_t *item) {
    not_null_void(conn);
    not_null_void(item);

    ssize_t ret = read(item->remote_fd, item->rd_buf + item->rd_buf_len,
                       CONN_BUF_SIZE - item->rd_buf_len);
    if (ret > 0) {
        item->rd_buf_len += ret;
        if (item->rd_buf_len >= CONN_BUF_SIZE) {
            /* Geocoding sources usually are trusted, but checking the
             * buffer length is still a good thing. */
            _reset_item(item);
            return;
        }
        item->rd_buf[item->rd_buf_len - 1] = '\0';
    }
    else if (ret == 0) {
        if (item->rd_buf_len) {
            gc_chomp(item->rd_buf, item->rd_buf_len);
            if (sscanf(item->rd_buf, "%d,%c,%lf,%lf",
                       &(item->result.code),
                       &(item->result.accuracy),
                       &(item->result.latitude),
                       &(item->result.longitude)) != 4) {
                _reset_item(item);
                return;
            }
            snprintf(item->wr_buf, CONN_BUF_SIZE, GEOCODING_OUTPUT_FMT,
                     item->result.code, item->result.accuracy,
                     item->result.latitude, item->result.longitude);
            item->wr_buf_len = strlen(item->wr_buf);
            item->wr_buf_pos = 0;

            if (gc_db_put(conn->db, item->location, &(item->result)) != 0) {
                gc_loge("Cannot put data into database");
            }
            if (close(item->remote_fd) != 0) {
                gc_loge("Cannot close remote fd");
            }
            item->remote_fd = -1;
            item->status = CONN_ST_REMOTE_CLOSED;
        }
        else {
            _reset_item(item);
        }
    }
    else if (ret < 0 && errno != EINPROGRESS) {
        gc_loge("Cannot read data from remote: %m");
        _reset_item(item);        
    }
}

static void _write_response(struct gc_conn_t *conn,
                            struct gc_conn_item_t *item) {
    not_null_void(conn);
    not_null_void(item);

    ssize_t ret = write(item->client_fd,
                        item->wr_buf + item->wr_buf_pos,
                        item->wr_buf_len - item->wr_buf_pos);
    if (ret > 0) {
        item->wr_buf_pos += ret;
    }
    else if (ret == 0) {
        _reset_item(item);
    }
    else if (ret < 0 && errno != EINPROGRESS) {
        gc_loge("Cannot write response to client: %m");
        _reset_item(item);
    }
}
            
int gc_conn_init(struct gc_conn_t **conn, size_t size) {
    not_null(conn);

    register size_t i = 0;
    struct hostent *host = NULL;
    
    if (!size) {
        return -1;
    }
    
    *conn = malloc(sizeof(struct gc_conn_t));
    if (*conn == NULL) {
        return -1;
    }

    (*conn)->items = malloc(size * sizeof(struct gc_conn_item_t));
    if ((*conn)->items == NULL) {
        safefree(*conn);
        return -1;
    }

    (*conn)->internal = malloc(sizeof(struct gc_conn_internal_t));
    if ((*conn)->internal == NULL) {
        safefree((*conn)->items);
        safefree(*conn);
        return -1;
    }

    (*conn)->size = size;

    memset((*conn)->items, 0, size * sizeof(struct gc_conn_item_t));

    for (i = 0; i < size; ++i) {
        (*conn)->items[i].client_fd = -1;
        (*conn)->items[i].remote_fd = -1;
    }

    /* Resolve host name */
    host = gethostbyname(GMAP_SERVER_HOSTNAME);
    if (host) {
        (*conn)->internal->gmap_server_count
            = GC_MIN(host->h_length, GMAP_SERVER_MAX_COUNT);
        for (i = 0; i < GC_MIN(host->h_length,
                               GMAP_SERVER_MAX_COUNT); ++i) {
            memcpy(&((*conn)->internal->gmap_servers[i]),
                   host->h_addr_list[i], sizeof(in_addr_t));
        }
    }
    else {
        gc_loge("Cannot find any map server: %s", hstrerror(h_errno));
        return -1;
    }

    return 0;
}

int gc_conn_add(struct gc_conn_t *conn, int fd, unsigned int timeout) {
    not_null(conn);

    register size_t i = 0;
    struct gc_conn_item_t *item = NULL;

    if (timeout > 60) {
        timeout = 60;
    }
    
    for (i = 0; i < conn->size; ++i) {
        item = &(conn->items[i]);
        if (item->status == CONN_ST_NULL) {
            item->client_fd = fd;
            item->exptime = time(NULL) + timeout;
            item->status = CONN_ST_INIT;
            return 0;
        }
    }

    return -1;                  /* No place for fd */
}

size_t gc_conn_process(struct gc_conn_t *conn) {
    if (conn == NULL) {
        return 0;
    }

    struct {
        void (*func_ptr)(struct gc_conn_t *conn,
                         struct gc_conn_item_t *item);
    } func_table[5] = {
        { _read_request },
        { _open_remote },
        { _write_remote },
        { _read_remote },
        { _write_response }
    };
    
    register size_t i = 0;
    struct gc_conn_item_t *item = NULL;
    size_t proc_count = 0;
    int ret = 0;
    int max_fd = -1;
    fd_set rdfds;
    fd_set wrfds;
    struct timeval tv;
    time_t curtime;

    curtime = time(NULL);
    FD_ZERO(&rdfds);
    FD_ZERO(&wrfds);
    
    for (i = 0; i < conn->size; ++i) {
        item = &(conn->items[i]);

        if (item->status == CONN_ST_NULL) {
            continue;
        }

        if (curtime > item->exptime) {
            _reset_item(item);
            continue;
        }

        if (item->client_fd >= 0) {
            if (item->status == CONN_ST_INIT) {
                FD_SET(item->client_fd, &rdfds);
            }
            if (item->status == CONN_ST_REMOTE_CLOSED) {
                FD_SET(item->client_fd, &wrfds);
            }
            if (item->client_fd > max_fd) {
                max_fd = item->client_fd;
            }
        }

        if (item->remote_fd >= 0) {
            if (item->status == CONN_ST_REMOTE_OPENED) {
                FD_SET(item->remote_fd, &wrfds);
            }
            if (item->status == CONN_ST_FORWARDED) {
                FD_SET(item->remote_fd, &rdfds);
            }
            if (item->remote_fd > max_fd) {
                max_fd = item->remote_fd;
            }
        }
    }

    if (max_fd < 0) {
        return 0;
    }
    tv.tv_sec = 0;
    tv.tv_usec = 1;

    ret = select(max_fd + 1, &rdfds, &wrfds, NULL, &tv);
    if (ret == -1) {
        if (errno == EINTR) {
            return 0;
        }
        gc_loge("Cannot select file descriptors: %m");
        return 0;
    }

    for (i = 0; i < conn->size; ++i) {
        item = &(conn->items[i]);
        if (item->status == CONN_ST_NULL) {
            continue;
        }
        if ((item->status == CONN_ST_INIT
             && item->client_fd >= 0
             && FD_ISSET(item->client_fd, &rdfds))
            || item->status == CONN_ST_GOT_REQUEST
            || (item->status == CONN_ST_REMOTE_OPENED
                && item->remote_fd >= 0
                && FD_ISSET(item->remote_fd, &wrfds))
            || (item->status == CONN_ST_FORWARDED
                && item->remote_fd >= 0
                && FD_ISSET(item->remote_fd, &rdfds))
            || (item->status == CONN_ST_REMOTE_CLOSED
                && item->client_fd >= 0
                && FD_ISSET(item->client_fd, &wrfds))) {
            gc_debug(printf("%d: Func: %d\n", i, item->status - 1));
            (func_table[item->status - 1].func_ptr)(conn, item);
            ++proc_count;
        }

        if (proc_count == ret) { /* We've processed enough file
                                  * descriptors. */
            break;
        }
    }

    return proc_count;
}

int gc_conn_load_key_file(struct gc_conn_t *conn, const char *filename) {
    not_null(conn);
    not_null(filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        gc_loge("Cannot open key file: %m");
        return -1;
    }
    if (fscanf(fp, "%s", conn->internal->gmap_key) != 1) {
        gc_loge("Cannot load key: %m");
        return -1;
    }
    if (fclose(fp) != 0) {
        gc_loge("Cannot close key file: %m");
        return -1;
    }
    return 0;
}

int gc_conn_free(struct gc_conn_t *conn) {
    not_null(conn);

    safefree(conn->items);
    safefree(conn);

    return 0;
}
