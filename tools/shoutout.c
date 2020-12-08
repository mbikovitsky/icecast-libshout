/*
 *  Copyright (C) 2020      Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
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
static int flag = 0;
#define FLAG_PROTO 1
#define FLAG_MOUNT 2
#define FLAG_USER  3
#define FLAG_PASS  4

static const struct option opts[] = {
    /* connection options */
    {"proto", required_argument, &flag, FLAG_PROTO},
    {"host",  required_argument, NULL, 'h'},
    {"port",  required_argument, NULL, 'p'},
    {"mount", required_argument, &flag, FLAG_MOUNT},
    {"user",  required_argument, &flag, FLAG_USER},
    {"pass",  required_argument, &flag, FLAG_PASS},
    {NULL,    0,                 NULL,  0},
};

static unsigned int string2proto(const char *name)
{
    if (strcmp(name, "http") == 0) return SHOUT_PROTOCOL_HTTP;
    if (strcmp(name, "icy") == 0)  return SHOUT_PROTOCOL_ICY;
    if (strcmp(name, "roar") == 0) return SHOUT_PROTOCOL_ROARAUDIO;

    return SHOUTERR_INSANE;
}

int main (int argc, char *argv[])
{
    /* default connection options */
    unsigned int proto = SHOUT_PROTOCOL_HTTP;
    const char *host = "127.0.0.1";
    int port = 8000;
    const char *mount = "/example.ogg";
    const char *user = "source";
    const char *pass = "hackme";

    unsigned char buf[4096];
    int c = 0;
    size_t nread = 0;
    int opt_index = 0;
    shout_t *shout;

    while ((c = getopt_long(argc, argv, "h:p:", opts, &opt_index)) != -1) {
        switch (c) {
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 0: /* long-only option */
                switch (flag) {
                    case FLAG_PROTO:
                        proto = string2proto(optarg);
                        break;
                    case FLAG_MOUNT:
                        mount = optarg;
                        break;
                    case FLAG_USER:
                        user = optarg;
                        break;
                    case FLAG_PASS:
                        pass = optarg;
                        break;
                    default:
                        fprintf(stderr, "unhandled flag (%d)\n", flag);
                        return EXIT_FAILURE;
                        break;
                }
                break;
            default:
                /* TODO: usage() */
                return EXIT_FAILURE;
                break;
        }
    }

    shout_init();

    if (!(shout = shout_new())) {
        printf("Could not allocate shout_t\n");
        return EXIT_FAILURE;
    }

    if (shout_set_protocol(shout, proto) != SHOUTERR_SUCCESS) {
        printf("Error setting protocol: %s\n", shout_get_error(shout));
        return EXIT_FAILURE;
    }

    if (shout_set_host(shout, host) != SHOUTERR_SUCCESS) {
        printf("Error setting hostname: %s\n", shout_get_error(shout));
        return EXIT_FAILURE;
    }

    if (shout_set_port(shout, port) != SHOUTERR_SUCCESS) {
        printf("Error setting port: %s\n", shout_get_error(shout));
        return EXIT_FAILURE;
    }

    if (shout_set_mount(shout, mount) != SHOUTERR_SUCCESS) {
        printf("Error setting mount: %s\n", shout_get_error(shout));
        return EXIT_FAILURE;
    }

    if (shout_set_user(shout, user) != SHOUTERR_SUCCESS) {
        printf("Error setting user: %s\n", shout_get_error(shout));
        return EXIT_FAILURE;
    }

    if (shout_set_password(shout, pass) != SHOUTERR_SUCCESS) {
        printf("Error setting password: %s\n", shout_get_error(shout));
        return EXIT_FAILURE;
    }

    if (shout_set_format(shout, SHOUT_FORMAT_OGG) != SHOUTERR_SUCCESS) {
        printf("Error setting format: %s\n", shout_get_error(shout));
        return EXIT_FAILURE;
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
