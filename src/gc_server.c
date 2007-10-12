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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "gc_error.h"
#include "gc_log.h"
#include "gc_util.h"
#include "gc_server.h"

extern int g_is_daemon;

int gc_server_setup(int port) {
    int fd = 0;
    int reuseaddr_on = 1;

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        gc_loge("Cannot open server socket: %m");
        return -1;
    }

    struct sockaddr_in s_in;
    memset(&s_in, 0, sizeof(s_in));
    s_in.sin_family = AF_INET;
    s_in.sin_port = htons(port);
    s_in.sin_addr.s_addr = INADDR_ANY;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on,
                   sizeof(reuseaddr_on)) == -1) {
        gc_loge("Cannot set socket options: %m");
        return -1;
    }
    if (gc_set_nonblock(fd) != 0) {
        gc_loge("Cannot set socket to nonblocking mode: %m");
        return -1;
    }
    if (bind(fd, (struct sockaddr *) &s_in, sizeof(s_in)) < 0) {
        gc_loge("Cannot bind socket: %m");
        return -1;
    }
    if (listen(fd, 20) < 0) {
        gc_loge("Cannot listen to connections: %m");
        return -1;
    }

    return fd;
}

