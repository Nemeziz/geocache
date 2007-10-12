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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#include "gc_debug.h"
#include "gc_error.h"
#include "gc_log.h"
#include "gc_util.h"

extern int g_is_daemon;

size_t gc_chomp(char *buf, size_t buf_size) {
    if (!buf) {
        return buf_size;
    }
    
    register size_t i = 0;

    buf[buf_size - 1] = '\0';

    for (i = 0; i < buf_size; ++i) {
        if (buf[i] == '\r' || buf[i] == '\n') {
            buf[i] = '\0';
            return i;
        }
    }
    
    return buf_size;
}

int gc_socket_connect(in_addr_t host, int port) {
    int fd = 0;
    int ret = 0;
    struct sockaddr_in server_addr;

    if ((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        gc_loge("Cannot open client socket: %m");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = host;
    server_addr.sin_port = htons(port);

    if (gc_set_nonblock(fd) != 0) {
        gc_loge("Cannot set socket to nonblocking mode: %m");
        if (close(fd) != 0) {
            gc_loge("Cannot close socket: %m");
            return -1;
        }
        return -1;
    }

    ret = connect(fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (ret < 0 && errno != EINPROGRESS) {
        gc_loge("Cannot connect to remote host: %m");
        if (close(fd) != 0) {
            gc_loge("Cannot close socket: %m");
            return -1;
        }
        return -1;
    }

    return fd;
}

int gc_set_nonblock(int fd) {
    int f = 0;

    f = fcntl(fd, F_GETFL, 0);
    if (f < 0) {
        return -1;
    }

    f = fcntl(fd, F_SETFL, f | O_NONBLOCK);
    if (f < 0) {
        return -1;
    }
    
    return 0;
}

size_t gc_uri_get_escaped_size(char *buf, size_t buf_size) {
    size_t size = 0;
    return size;
}

size_t gc_get_path_of(const char *filename, char *buf, size_t buf_size) {
    not_null(filename);

    size_t filename_len = strlen(filename);
    register int i = filename_len - 1;

    for (; i >= 0; --i) {
        if (filename[i] == '/') {
            break;
        }
    }
    if (i > 0) {
        strncpy(buf, filename, i);
        buf[i] = '\0';
    }
    else {
        buf[0] = '.';
        buf[1] = '\0';
    }
        
    return strlen(buf);
}
