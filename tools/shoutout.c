/*
 *  Copyright (C) 2020      Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
 *  Copyright (C) 2020      Jakob Wied <jawied@de.loewenfelsen.net>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <shout/shout.h>

/* only long options need a flag value */
#define FLAG_PROTO 1
#define FLAG_MOUNT 2
#define FLAG_USER  3
#define FLAG_PASS  4

void usage_oggfwd(const char *progname)
{
    fprintf(stderr,
        "Usage: %s [OPTIONS] address port password mountpoint\n"
        "\n"
        "OPTIONS:\n"
        "  -d <description>    set stream description\n"
        "  -g <genre>          set stream genre\n"
        "  -h                  show this help\n"
        "  -m <file>           parse metadata from file\n"
        "  -n <name>           set stream name\n"
        "  -p                  make stream public\n"
        "  -u <url>            set stream url\n"
        , progname);
}

static unsigned int string2proto(const char *name)
{
    if (strcmp(name, "http") == 0) return SHOUT_PROTOCOL_HTTP;
    if (strcmp(name, "icy") == 0)  return SHOUT_PROTOCOL_ICY;
    if (strcmp(name, "roar") == 0) return SHOUT_PROTOCOL_ROARAUDIO;

    return SHOUTERR_INSANE;
}

static int getopts_shout(int argc, char *argv[], shout_t *shout)
{
    int flag = 0;
    const struct option possible[] = {
        /* connection options */
        {"proto", required_argument, &flag, FLAG_PROTO},
        {"host",  required_argument, NULL, 'h'},
        {"port",  required_argument, NULL, 'p'},
        {"mount", required_argument, &flag, FLAG_MOUNT},
        {"user",  required_argument, &flag, FLAG_USER},
        {"pass",  required_argument, &flag, FLAG_PASS},
        {NULL,    0,                 NULL,  0},
    };

    int port;
    int proto;
    int c;
    int i = 0;

    while ((c = getopt_long(argc, argv, "h:p:", possible, &i)) != -1) {
        switch (c) {
            case 'h':
                if (shout_set_host(shout, optarg) != SHOUTERR_SUCCESS) {
                    printf("Error setting hostname: %s\n",
                            shout_get_error(shout));
                    return -1;
                }
                break;

            case 'p':
                port = atoi(optarg);
                if (shout_set_port(shout, port) != SHOUTERR_SUCCESS) {
                    printf("Error setting port: %s\n",
                            shout_get_error(shout));
                    return -1;
                }
                break;

            case 0: /* long-only option */
                switch (flag) {
                    case FLAG_PROTO:
                        proto = string2proto(optarg);
                        if (shout_set_protocol(shout, proto) !=
                                SHOUTERR_SUCCESS) {
                            printf("Error setting protocol: %s\n",
                                    shout_get_error(shout));
                            return -1;
                        }
                        break;
                    case FLAG_MOUNT:
                        if (shout_set_mount(shout, optarg) !=
                                SHOUTERR_SUCCESS) {
                            printf("Error setting mount: %s\n",
                                    shout_get_error(shout));
                            return -1;
                        }
                        break;
                    case FLAG_USER:
                        if (shout_set_user(shout, optarg) !=
                                SHOUTERR_SUCCESS) {
                            printf("Error setting user: %s\n",
                                    shout_get_error(shout));
                            return -1;
                        }
                        break;
                    case FLAG_PASS:
                        if (shout_set_password(shout, optarg) !=
                                SHOUTERR_SUCCESS) {
                            printf("Error setting password: %s\n",
                                    shout_get_error(shout));
                            return -1;
                        }
                        break;
                    default:
                        fprintf(stderr, "unhandled flag (%d)\n", flag);
                        return -1;
                        break;
                }
                break;
            default: /* unknown short option */
                /* TODO: usage() */
                return -1;
                break;
        }
    }
    return 0;
}

int main (int argc, char *argv[])
{
    unsigned char buf[4096];
    size_t nread = 0;
    shout_t *shout;

    shout_init();

    if (!(shout = shout_new())) {
        printf("Could not allocate shout_t\n");
        return EXIT_FAILURE;
    }

    if (getopts_shout(argc, argv, shout)) {
        return EXIT_FAILURE;
    }

    /* mount is not set by shout_new */
    if (!shout_get_mount(shout)) {
        if (shout_set_mount(shout, "/example.ogg") != SHOUTERR_SUCCESS) {
            printf("Error setting mount: %s\n", shout_get_error(shout));
            return EXIT_FAILURE;
        }
    }

    /* format is not set by shout_new */
    if(!shout_get_format(shout)) {
        if (shout_set_format(shout, SHOUT_FORMAT_OGG) != SHOUTERR_SUCCESS) {
            printf("Error setting format: %s\n", shout_get_error(shout));
            return EXIT_FAILURE;
        }
    }

    /* password is not set by shout_new */
    if (!shout_get_password(shout)) {
        if (shout_set_password(shout, "hackme") != SHOUTERR_SUCCESS) {
            printf("Error setting password: %s\n", shout_get_error(shout));
            return EXIT_FAILURE;
        }
    }

    if (shout_open(shout) != SHOUTERR_SUCCESS) {
        printf("Error connecting: %s\n", shout_get_error(shout));
        return EXIT_FAILURE;
    }

    while ((nread = fread(buf, 1, sizeof(buf), stdin)) > 0) {
        if(shout_send(shout, buf, nread) != SHOUTERR_SUCCESS) {
            printf("Error sending: %s\n", shout_get_error(shout));
            return EXIT_FAILURE;
        }

        shout_sync(shout);
    }

    shout_close(shout);
    shout_shutdown();

    /* don't return 0 if the loop isn't terminated by EOF */
    if (feof(stdin) == 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
