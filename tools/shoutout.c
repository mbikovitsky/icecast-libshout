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
int flag = 0;
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

int main (int argc, char *argv[])
{
    /* default connection options */
    unsigned int proto = SHOUT_PROTOCOL_HTTP;
    const char *host = "127.0.0.1";
    int port = 8000;
    const char *mount = "/example.ogg";
    const char *user = "source";
    const char *pass = "hackme";

    int c = 0;
    int opt_index = 0;

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
                        if (strcmp(optarg, "http") == 0) {
                            proto = SHOUT_PROTOCOL_HTTP;
                        } else {
                            fprintf(stderr, "%s: invalid protocol\n", optarg);
                            return EXIT_FAILURE;
                        }
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

    return 0;
}
