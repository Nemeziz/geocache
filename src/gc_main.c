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
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>

#include <config.h>

#include "gc_debug.h"
#include "gc_error.h"
#include "gc_log.h"
#include "gc_db.h"
#include "gc_server.h"
#include "gc_conn.h"
#include "gc_util.h"

#define FILENAME_SIZE 64
#define PROG_NAME PACKAGE_NAME

struct gc_main_t {
    int server_fd;
    int port;
    unsigned int timeout;
    struct gc_db_t *db;
    struct gc_conn_t *conn;
    char db_filename[FILENAME_SIZE];
    char key_filename[FILENAME_SIZE];
    char pid_filename[FILENAME_SIZE];
};

extern char *optarg;

static struct gc_main_t gs_gc;

int g_is_daemon = 0;

static int _kill_daemon(struct gc_main_t *gc, int signum) {
    not_null(gc);

    FILE *fp = NULL;
    pid_t pid = 0;

    fp = fopen(gc->pid_filename, "r");
    if (!fp) {
        gc_loge("Cannot open pid file: %m");
        return -1;
    }
    if (fscanf(fp, "%d", &pid) != 1) {
        gc_loge("Cannot load pid: %m");
        return -1;
    }
    if (fclose(fp) != 0) {
        gc_loge("Cannot close pid file: %m");
        return -1;
    }

    if (kill(pid, !signum ? SIGTERM : signum ) != 0) {
        gc_loge("Cannot kill daemon: %m");
        return -1;
    }
    return 0;
}

static void _dbsync_handler(int signum) {
    sigset_t mask_set;
    sigset_t old_set;

    if (signal(signum, _dbsync_handler) == SIG_ERR) {
        gc_loge("Cannot set signal handler: %m");
    }
    sigfillset(&mask_set);
    sigprocmask(SIG_SETMASK, &mask_set, &old_set);

    gc_log("Receiving db sync signal.");
    if (gc_db_sync(gs_gc.db) != 0) {
        gc_loge("Cannot sync database: %m");
    }
    gc_log("Database is syncked");
}

static void _termination_handler(int signum) {
    sigset_t mask_set;
    sigset_t old_set;

    if (signal(signum, _termination_handler) == SIG_ERR) {
        gc_loge("Cannot set signal handler: %m");
    }
    sigfillset(&mask_set);
    sigprocmask(SIG_SETMASK, &mask_set, &old_set);

    /* Sync database */
    if (gc_db_sync(gs_gc.db) != 0) {
        gc_loge("Cannot sync database: %m");
    }

    /* Remove pid file */
    if (unlink(gs_gc.pid_filename) != 0) {
        gc_loge("Cannot remove pid file '%s': %m", gs_gc.pid_filename);
    }
    
    if (gc_db_free(gs_gc.db) != 0) {
        gc_loge("Cannot free database: %m");
    }
    if (gc_conn_free(gs_gc.conn) != 0) {
        gc_loge("Cannot free connections: %m");
    }

    gc_log("Program terminated");
    
    exit(0);
}

static void _parse_opts(int argc, char *argv[], struct gc_main_t *gc) {
    not_null_void(gc);
    
    int opt = 0;

    g_is_daemon = 0;

    /* Set up default values */
    gc->port = 1732;
    gc->timeout = 5;
    snprintf(gc->db_filename,
             FILENAME_SIZE, "%s", "/var/lib/" PROG_NAME "/" PROG_NAME ".db");
    snprintf(gc->key_filename,
             FILENAME_SIZE, "%s", "/etc/" PROG_NAME "/google.key");
    snprintf(gc->pid_filename,
             FILENAME_SIZE, "%s", "/var/run/" PROG_NAME ".pid");

    while ((opt = getopt(argc, argv, "d:k:P:p:t:DvKSh")) != -1) {
        switch (opt) {
            case 'd': {
                snprintf(gc->db_filename, FILENAME_SIZE, "%s", optarg);
                break;
            }
            case 'k': {
                snprintf(gc->key_filename, FILENAME_SIZE, "%s", optarg);
                break;
            }
            case 'P': {
                snprintf(gc->pid_filename, FILENAME_SIZE, "%s", optarg);
                break;
            }
            case 'p': {
                gc->port = atoi(optarg);
                break;
            }
            case 't': {
                gc->timeout = atoi(optarg);
                break;
            }
            case 'D': {
                g_is_daemon = 1;
                break;
            }
            case 'K': {
                gc_log("Sending termination signal");
                if (_kill_daemon(gc, 0) != 0) {
                    gc_loge("Cannot terminate database");
                    exit(-1);
                }
                else {
                    gc_log("Program terminated");
                    exit(-1);
                }
                break;
            }
            case 'S': {
                gc_log("Sending signal to sync database");
                if (_kill_daemon(gc, SIGHUP) != 0) {
                    gc_loge("Cannot sync database");
                    exit(-1);
                }
                else {
                    gc_log("Database is syncked");
                    exit(0);
                }
                break;
            }
            case 'v': {
                fprintf(stderr, PROG_NAME " " VERSION "\n");
                exit(0);
            }
            default: {
                fprintf(stderr,
                        PROG_NAME "\n"
                        "\n"
                        "    -d database\n"
                        "    -k file of google key\n"
                        "    -p port (Default: 1732)\n"
                        "    -P pid_file\n"
                        "    -t timeout value (in seconds) (Default: 5 seconds)\n"
                        "    -K (kill the running daemon)\n"
                        "    -S (sync database)\n"
                        "    -D (run as a daemon)\n"
                        "    -v (show version)\n"
                        "    -h (show version)\n"
                        "\n");
                exit(0);
            }
        }
    }
}

static void _become_daemon(struct gc_main_t *gc) {
    not_null_void(gc);

    pid_t pid = 0;
    pid_t sid = 0;

    pid = fork();
    if (pid < 0) {
        gc_loge("Cannot fork(): %m");
        exit(-1);
    }
    if (pid > 0) {
        exit(0);
    }
    umask(0);

    sid = setsid();
    if (sid < 0) {
        gc_loge("Cannot setsid(): %m");
        exit(-1);
    }
    
    if (chdir("/") != 0) {
        gc_loge("Cannot setsid(): %m");
        exit(-1);
    }
    
    if (close(STDIN_FILENO) != 0) {
        gc_loge("Cannot close STDIN: %m");
        exit(-1);
    }
    if (close(STDOUT_FILENO) != 0) {
        gc_loge("Cannot close STDOUT: %m");
        exit(-1);
    }
    if (close(STDERR_FILENO) != 0) {
        gc_loge("Cannot close STDERR: %m");
        exit(-1);
    }
}

static void _write_pid_file(struct gc_main_t *gc) {
    not_null_void(gc);

    FILE *fp = NULL;

    umask(0022);
    fp = fopen(gc->pid_filename, "w");
    if (!fp) {
        gc_loge("Cannot open pid file: %m");
        exit(-1);
    }
    fprintf(fp, "%d\n", getpid());
    if (fclose(fp) != 0) {
        gc_loge("Cannot close pid file: %m");
        exit(-1);
    }
    umask(0);
}

static void _set_signal_handlers(struct gc_main_t *gc) {
    not_null_void(gc);

    if (signal(SIGINT, _termination_handler) == SIG_ERR) {
        gc_loge("Cannot set signal handler: %m");
        exit(-1);
    }
    if (signal(SIGTERM, _termination_handler) == SIG_ERR) {
        gc_loge("Cannot set signal handler: %m");
        exit(-1);
    }
    if (signal(SIGHUP, _dbsync_handler) == SIG_ERR) {
        gc_loge("Cannot set signal handler: %m");
        exit(-1);
    }
}

static void _initialize_gc(struct gc_main_t *gc) {
    not_null_void(gc);
    
    size_t conn_size = 500;

    if (gc_db_init(&(gc->db)) != 0) {
        gc_loge("Cannot initialize database");
        exit(-1);
    }
    not_null_void(gc->db);

    gc_db_load(gc->db, gc->db_filename);

    if (gc_conn_init(&(gc->conn), conn_size) != 0) {
        gc_loge("Cannot initialize connection");
        exit(-1);
    }
    not_null_void(gc->conn);
    gc->conn->db = gc->db;

    if (gc_conn_load_key_file(gc->conn, gc->key_filename) != 0) {
        gc_loge("Cannot load key file");
        exit(-1);
    }
    
    gc->server_fd = gc_server_setup(gc->port);
    if (gc->server_fd < 0) {
        gc_loge("Cannot set up server: %m");
        exit(-1);
    }

    _set_signal_handlers(gc);
}

static void _process_requests(struct gc_main_t *gc) {
    not_null_void(gc);

    int client_fd = 0;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    if (gc_set_nonblock(gc->server_fd) != 0) {
        exit(-1);
    }
    while (1) {
        gc_conn_process(gc->conn);
        client_fd = accept(gc->server_fd, (struct sockaddr *) &client_addr,
                           &client_len);
        if (client_fd < 0 && errno != EAGAIN) {
            continue;
        }
        
        if (gc_set_nonblock(client_fd) != 0) {
            if (close(client_fd) != 0) {
            }
            continue;
        }

        if (gc_conn_add(gc->conn, client_fd, gc->timeout) != 0) {
            gc_loge("Cannot add new connection");
        }
    }
}

int main(int argc, char *argv[]) {
    struct stat stbuf;
    
    srand(time(NULL));
    
    _parse_opts(argc, argv, &gs_gc);

    if (stat(gs_gc.pid_filename, &stbuf) == 0) {
        fprintf(stderr, PROG_NAME " is already running\n");
        exit(-1);
    }

    printf("db file: %s\n"
           "key file: %s\n"
           "port: %d\n"
           "pid file: %s\n\n",
           gs_gc.db_filename, gs_gc.key_filename, gs_gc.port,
           gs_gc.pid_filename);
    
    if (!g_is_daemon) {
        openlog(PROG_NAME, LOG_NDELAY | LOG_PERROR, 0);
    }
    else {
        _become_daemon(&gs_gc);
        /* STDERR is already closed. Do not use LOG_PERROR. */
        openlog(PROG_NAME, LOG_NDELAY, 0);
    }
    
    gc_log(PROG_NAME " is started");

    _initialize_gc(&gs_gc);
    _write_pid_file(&gs_gc);
    _process_requests(&gs_gc);

    return 0;
}
