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
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <shout/shout.h>

/* only long options need a flag value */
enum flag {
    FLAG__NONE      = 0,
    FLAG_PROTO      = 1,
    FLAG_MOUNT      = 2,
    FLAG_USER       = 3,
    FLAG_PASS       = 4,
    FLAG_TLS_MODE   = 5
};

#ifdef SHOUT_TLS
static const char supported_tls_modes[] = "disabled|auto|auto_no_plain|rfc2818|rfc2817";
#else
static const char supported_tls_modes[] = "disabled";
#endif

static inline int string2proto(const char *name, unsigned int *res)
{
    if (!res)
        return -1;

    if (strcmp(name, "http") == 0) {
        *res = SHOUT_PROTOCOL_HTTP;
    } else if (strcmp(name, "icy") == 0) {
        *res = SHOUT_PROTOCOL_ICY;
    } else if (strcmp(name, "roar") == 0) {
        *res = SHOUT_PROTOCOL_ROARAUDIO;
    } else {
        return -1;
    }

    return 0;
}

static inline int string2tlsmode(const char *name, int *res)
{
    if (!res)
        return -1;

    if (strcasecmp(name, "disabled") == 0) {
        *res = SHOUT_TLS_DISABLED;
    } else if (strcasecmp(name, "auto") == 0) {
        *res = SHOUT_TLS_AUTO;
    } else if (strcasecmp(name, "auto_no_plain") == 0) {
        *res = SHOUT_TLS_AUTO_NO_PLAIN;
    } else if (strcasecmp(name, "rfc2818") == 0) {
        *res = SHOUT_TLS_RFC2818;
    } else if (strcasecmp(name, "rfc2817") == 0) {
        *res = SHOUT_TLS_RFC2817;
    } else {
        return -1;
    }

    return 0;
}

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
        "  –T {%s}\n"
        "                      set TLS mode\n"
        , progname, supported_tls_modes);
}

void usage_shout(const char *progname)
{
    fprintf(stderr,
        "Usage: %s [OPTIONS]\n"
        "\n"
        "OPTIONS:\n"
        "  -H <host>, --host <host>    set host\n"
        "  -h, --help                  show this help\n"
        "  --mount <mountpoint>        set mountpoint\n"
        "  -P <port>, --port <port>    set port\n"
        "  --pass <password>           set source password\n"
        "  --proto <protocol>          set protocol (e.g. \"http\")\n"
        "  --user <user>               set source user\n"
        "  --tls-mode <tls-mode>       set TLS mode {%s}\n"
        , progname, supported_tls_modes);
}

/* parse_metadata_file is called at `oggfwd -m`.
 * It fills the shout struct with information read from file.
 * It returns 0 on success, or -1 on error.
 */
static int parse_metadata_file(const char *path, shout_t *shout)
{
    FILE *fh;
    char line[256];
    int lineno = 0;
    size_t len;
    char *pos;
    char c;

    if ((fh = fopen(path, "r")) == NULL) {
        perror(path);
        return -1;
    }

    while (fgets(line, sizeof(line), fh)) {
        lineno++;

        len = strlen(line);
        if (len == 0) {
            continue;
        }

        /* skip comments and empty lines */
        c = line[0];
        if ((c == '#') || (c == '\r') || (c == '\n')) {
            continue;
        }

        /* strip line endings */
        if (line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        if (line[len-1] == '\r') {
            line[len-1] = '\0';
            len--;
        }

        /* find '=' */
        if ((pos = strchr(line, '=')) == NULL) {
            fprintf(stderr, "%s:%d: syntax error\n", path, lineno);
            continue; /* oggfwd doesn't abort */
        }

        *pos = '\0'; /* terminate key */
        pos++;

        /* oggfwd ignores the result of shout_set_* */
        if (strcmp(line, SHOUT_META_DESCRIPTION) == 0) {
            shout_set_meta(shout, SHOUT_META_DESCRIPTION, pos);
        } else if (strcmp(line, SHOUT_META_GENRE) == 0) {
            shout_set_meta(shout, SHOUT_META_GENRE, pos);
        } else if (strcmp(line, SHOUT_META_NAME) == 0) {
            shout_set_meta(shout, SHOUT_META_NAME, pos);
        } else if (strcmp(line, SHOUT_META_URL) == 0) {
            shout_set_meta(shout, SHOUT_META_URL, pos);
        } else {
            /* NOTE: Maybe we should try to set invalid keys.
             * However, this wouldn't be consistent with the cli options
             * which only allow description, genre, name, and url.
             */
            fprintf(stderr, "%s:%d: \"%s\" is not a valid key\n",
                    path, lineno, line);
            continue; /* oggfwd doesn't abort */
        }
    }

    if (feof(fh) == 0) {
        perror(path);
        fclose(fh);
        return -1;
    }

    fclose(fh);
    return 0;
}

static int getopts_oggfwd(int argc, char *argv[], shout_t *shout)
{
    const int ok = SHOUTERR_SUCCESS; /* helps to keep lines at 80 chars */
    int c;
    int tls_mode;

    while ((c = getopt(argc, argv, "d:g:hm:n:pu:T:")) != -1) {
        switch (c) {
            case 'd':
                if (shout_set_meta(shout, SHOUT_META_DESCRIPTION, optarg) != ok) {
                    printf("Error setting description: %s\n",
                            shout_get_error(shout));
                    return -1;
                }
                break;

            case 'g':
                if (shout_set_meta(shout, SHOUT_META_GENRE, optarg) != ok) {
                    printf("Error setting genre: %s\n",
                            shout_get_error(shout));
                    return -1;
                }
                break;

            case 'h':
                usage_oggfwd(argv[0]);
                return -1; /* stop further processing */

            case 'm':
                if (parse_metadata_file(optarg, shout) != 0) {
                    return -1;
                }
                break;

            case 'n':
                if (shout_set_meta(shout, SHOUT_META_NAME, optarg) != ok) {
                    printf("Error setting name: %s\n",
                            shout_get_error(shout));
                    return -1;
                }
                break;

            case 'p':
                if (shout_set_public(shout, 1) != ok) {
                    printf("Error setting public: %s\n",
                            shout_get_error(shout));
                    return -1;
                }
                break;

            case 'u':
                if (shout_set_meta(shout, SHOUT_META_URL, optarg) != ok) {
                    printf("Error setting url: %s\n",
                            shout_get_error(shout));
                    return -1;
                }
                break;
            case 'T':
                if (string2tlsmode(optarg, &tls_mode) != 0) {
                    printf("Error parsing TLS mode: %s: Invalid protocol name", optarg);
                    return -1;
                }

                if (shout_set_tls(shout, tls_mode) != ok) {
                    printf("Error setting TLS mode: %s\n",
                            shout_get_error(shout));
                    return -1;
                }
                break;

            default:
                usage_oggfwd(argv[0]);
                return -1;
        }
    }

    /* need exacly 4 remaining arguments */
    if ((argc - optind) != 4) {
        fprintf(stderr, "Wrong number of arguments\n");
        usage_oggfwd(argv[0]);
        return -1;
    }

    if (shout_set_host(shout, argv[optind++]) != SHOUTERR_SUCCESS) {
        printf("Error setting hostname: %s\n", shout_get_error(shout));
        return -1;
    }

    if (shout_set_port(shout, atoi(argv[optind++])) != SHOUTERR_SUCCESS) {
        printf("Error setting port: %s\n", shout_get_error(shout));
        return -1;
    }

    if (shout_set_password(shout, argv[optind++]) != SHOUTERR_SUCCESS) {
        printf("Error setting password: %s\n", shout_get_error(shout));
        return -1;
    }

    if (shout_set_mount(shout, argv[optind++]) != SHOUTERR_SUCCESS) {
        printf("Error setting mount: %s\n", shout_get_error(shout));
        return -1;
    }

    return 0;
}

static int getopts_shout(int argc, char *argv[], shout_t *shout)
{
    int flag = FLAG__NONE;
    const struct option possible[] = {
        /* connection options */
        {"proto",       required_argument, &flag, FLAG_PROTO},
        {"host",        required_argument, NULL, 'H'},
        {"port",        required_argument, NULL, 'P'},
        {"mount",       required_argument, &flag, FLAG_MOUNT},
        {"user",        required_argument, &flag, FLAG_USER},
        {"pass",        required_argument, &flag, FLAG_PASS},
        {"tls-mode",    required_argument, &flag, FLAG_TLS_MODE},
        /* other options */
        {"help",        no_argument,       NULL, 'h'},
        {NULL,          0,                 NULL,  0},
    };

    unsigned int proto;
    int tls_mode;
    int port;
    int c;
    int i = 0;

    while ((c = getopt_long(argc, argv, "H:hP:", possible, &i)) != -1) {
        switch (c) {
            case 'H':
                if (shout_set_host(shout, optarg) != SHOUTERR_SUCCESS) {
                    printf("Error setting hostname: %s\n",
                            shout_get_error(shout));
                    return -1;
                }
                break;

            case 'h':
                usage_shout(argv[0]);
                return -1;

            case 'P':
                port = atoi(optarg);
                if (shout_set_port(shout, port) != SHOUTERR_SUCCESS) {
                    printf("Error setting port: %s\n",
                            shout_get_error(shout));
                    return -1;
                }
                break;

            case 0: /* long-only option */
                switch ((enum flag)flag) {
                    case FLAG_PROTO:
                        if (string2proto(optarg, &proto) != 0) {
                            printf("Error parsing protocol: %s: Invalid protocol name\n", optarg);
                            return -1;
                        }
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
                    case FLAG_TLS_MODE:
                        if (string2tlsmode(optarg, &tls_mode) != 0) {
                            printf("Error parsing TLS mode: %s: Invalid protocol name", optarg);
                            return -1;
                        }

                        if (shout_set_tls(shout, tls_mode) !=
                                SHOUTERR_SUCCESS) {
                            printf("Error setting TLS mode: %s\n",
                                    shout_get_error(shout));
                            return -1;
                        }
                        break;
                    default:
                        usage_shout(argv[0]);
                        return -1;
                        break;
                }
                break;
            default: /* unknown short option */
                usage_shout(argv[0]);
                return -1;
                break;
        }
    }

    /* prohibit trailing arguments
     * NOTE: maybe we should treat them as input files
     */
    if (optind != argc) {
        for (; optind < argc; optind++) {
            fprintf(stderr, "%s: unused argument\n", argv[optind]);
        }
        fprintf(stderr, "\n"); /* don't stick usage to warnings */
        usage_shout(argv[0]);
        return -1;
    }

    return 0;
}

int main (int argc, char *argv[])
{
    unsigned char buf[4096];
    int err;
    size_t nread = 0;
    const char *progname; /* don't use __progname from glibc */
    shout_t *shout;

    shout_init();

    if (!(shout = shout_new())) {
        printf("Could not allocate shout_t\n");
        return EXIT_FAILURE;
    }

    if ((progname = basename(argv[0])) && (strcmp(progname, "oggfwd") == 0)) {
        err = getopts_oggfwd(argc, argv, shout);
    } else {
        err = getopts_shout(argc, argv, shout);
    }
    if (err) {
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
