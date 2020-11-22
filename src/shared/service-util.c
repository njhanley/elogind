/* SPDX-License-Identifier: LGPL-2.1+ */

#include <getopt.h>
#include <stdio.h>

#include "alloc-util.h"
#include "pretty-print.h"
#include "service-util.h"
#include "terminal-util.h"
#include "util.h"

static int help(const char *program_path, const char *service, const char *description, bool bus_introspect) {
        _cleanup_free_ char *link = NULL;
        int r;

        r = terminal_urlify_man(service, "8", &link);
        if (r < 0)
                return log_oom();

        printf("%s [OPTIONS...]\n\n"
               "%s%s%s\n\n"
               "This program takes no positional arguments.\n\n"
               "%sOptions%s:\n"
#if 1 /// elogind allows to be daemonized
               "  -D --daemon               Daemonize as a background service\n"
#endif // 1
               "  -h --help                 Show this help\n"
               "     --version              Show package version\n"
               "     --bus-introspect=PATH  Write D-Bus XML introspection data\n"
               "\nSee the %s for details.\n"
               , program_path
               , ansi_highlight(), description, ansi_normal()
               , ansi_underline(), ansi_normal()
               , link
        );

        return 0; /* No further action */
}

int service_parse_argv(
                const char *service,
                const char *description,
                const BusObjectImplementation* const* bus_objects,
                int argc, char *argv[]) {

        enum {
                ARG_VERSION = 0x100,
                ARG_BUS_INTROSPECT,
        };

        static const struct option options[] = {
#if 1 /// elogind allows to be daemonized
                { "daemon",         no_argument,       NULL, 'D'                },
#endif // 1
                { "help",           no_argument,       NULL, 'h'                },
                { "version",        no_argument,       NULL, ARG_VERSION        },
                { "bus-introspect", required_argument, NULL, ARG_BUS_INTROSPECT },
                {}
        };

        int c;

        assert(argc >= 0);
        assert(argv);

#if 0 /// Add options for elogind
        while ((c = getopt_long(argc, argv, "h", options, NULL)) >= 0)
#else // 0
        while ((c = getopt_long(argc, argv, "Dh", options, NULL)) >= 0)
#endif // 0
                switch(c) {

#if 1 /// elogind allows to be daemonized
                case 'D':
                        /* The option is already handled by elogind_startup() */
                        break;
#endif // 1

                case 'h':
                        return help(argv[0], service, description, bus_objects);

                case ARG_VERSION:
                        return version();

                case ARG_BUS_INTROSPECT:
                        return bus_introspect_implementations(
                                        stdout,
                                        optarg,
                                        bus_objects);

                case '?':
                        return -EINVAL;

                default:
                        assert_not_reached("Unknown option code.");
                }

        if (optind < argc)
                return log_error_errno(SYNTHETIC_ERRNO(EINVAL),
                                       "This program takes no arguments.");

        return 1; /* Further action */
}
