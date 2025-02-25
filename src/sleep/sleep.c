/* SPDX-License-Identifier: LGPL-2.1-or-later */
/***
  Copyright © 2010-2017 Canonical
  Copyright © 2018 Dell Inc.
***/

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/fiemap.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "sd-messages.h"

#include "btrfs-util.h"
#include "bus-error.h"
#include "def.h"
#include "exec-util.h"
#include "fd-util.h"
#include "fileio.h"
#include "format-util.h"
#include "io-util.h"
#include "log.h"
#include "main-func.h"
#include "parse-util.h"
#include "pretty-print.h"
#include "sleep-config.h"
#include "stdio-util.h"
#include "string-util.h"
#include "strv.h"
#include "time-util.h"
#include "util.h"

/// Additional includes needed by elogind
#include "exec-elogind.h"
#include "sd-login.h"
#include "sleep.h"
#include "terminal-util.h"
#include "utmp-wtmp.h"

static SleepOperation arg_operation = _SLEEP_OPERATION_INVALID;

#if 1 /// If an nvidia card is present, elogind informs its driver about suspend/resume actions
static int nvidia_sleep(Manager* m, SleepOperation operation, unsigned* vtnr) {
        static char const* drv_suspend = "/proc/driver/nvidia/suspend";
        int r;
        char** session;
        char** sessions;
        struct stat std;
        unsigned vt = 0;

        assert(operation < _SLEEP_OPERATION_INVALID);
        assert(vtnr);

        // See whether an nvidia suspension is possible
        r = stat(drv_suspend, &std);
        if (r)
                return 0;

        if (operation < _SLEEP_OPERATION_MAX) {
                *vtnr = 0;

                // Find the (active) sessions of the sleep sender
                r = sd_uid_get_sessions(m->scheduled_sleep_uid, 1, &sessions);
#if ENABLE_DEBUG_ELOGIND
                char *t = strv_join(sessions, " ");
                log_debug_elogind("sd_uid_get_sessions() returned %d, result is: %s", r, strnull(t));
                free(t);
#endif // elogind debug
                if (r < 0)
                        return 0;

                // Find one with a VT (Really, sessions should hold only one active session anyway!)
                STRV_FOREACH(session, sessions) {
                        int k;
                        k = sd_session_get_vt(*session, &vt);
                        if (k >= 0) {
                                log_debug_elogind("Active session %s with VT %u found", *session, vt);
                                break;
                        }
                }

                strv_free(sessions);

                // Get to a safe non-gui VT
                if ( (vt > 0) && (vt < 63) ) {
                        log_debug_elogind("Storing VT %u and switching to VT %d", vt, 63);
                        *vtnr = vt;
                        (void) chvt(63);
                }

                // Okay, go to sleep.
                if (operation == SLEEP_SUSPEND) {
                        log_debug_elogind("Writing 'suspend' into %s", drv_suspend);
                        r = write_string_file(drv_suspend, "suspend", WRITE_STRING_FILE_DISABLE_BUFFER);
                } else {
                        log_debug_elogind("Writing 'hibernate' into %s", drv_suspend);
                        r = write_string_file(drv_suspend, "hibernate", WRITE_STRING_FILE_DISABLE_BUFFER);
                }

                if (r)
                        return 0;
        } else {
                // Wakeup the device
                log_debug_elogind("Writing 'resume' into %s", drv_suspend);
                (void) write_string_file(drv_suspend, "resume", WRITE_STRING_FILE_DISABLE_BUFFER);
                // Then try to change back
                if (*vtnr > 0) {
                        log_debug_elogind("Switching back to VT %u", *vtnr);
                        (void) chvt(*vtnr);
                }
        }

        return 1;
}
#endif // 1

static int write_hibernate_location_info(const HibernateLocation *hibernate_location) {
        char offset_str[DECIMAL_STR_MAX(uint64_t)];
        char resume_str[DECIMAL_STR_MAX(unsigned) * 2 + STRLEN(":")];
        int r;
#if 1 /// To support LVM setups, elogind uses device numbers
        char device_num_str[DECIMAL_STR_MAX(uint32_t) * 2 + 2];
        struct stat stb;
#endif // 1

        assert(hibernate_location);
        assert(hibernate_location->swap);

        xsprintf(resume_str, "%u:%u", major(hibernate_location->devno), minor(hibernate_location->devno));
        r = write_string_file("/sys/power/resume", resume_str, WRITE_STRING_FILE_DISABLE_BUFFER);

#if 1 /// To support LVM setups, elogind uses device numbers if the direct approach failed
        if (r < 0) {
                r = stat(hibernate_location->swap->device, &stb);
                if (r < 0)
                        return log_debug_errno(errno, "Error while trying to get stats for %s: %m",
                                               hibernate_location->swap->device);

                (void) snprintf(device_num_str, DECIMAL_STR_MAX(uint32_t) * 2 + 2,
                                "%u:%u",
                                major(stb.st_rdev), minor(stb.st_rdev));
                r = write_string_file("/sys/power/resume", device_num_str, 0);
        }
#endif // 1

        if (r < 0)
                return log_debug_errno(r, "Failed to write partition device to /sys/power/resume for '%s': '%s': %m",
                                       hibernate_location->swap->device, resume_str);

        log_debug("Wrote resume= value for %s to /sys/power/resume: %s", hibernate_location->swap->device, resume_str);

        /* if it's a swap partition, we're done */
        if (streq(hibernate_location->swap->type, "partition"))
                return r;

        if (!streq(hibernate_location->swap->type, "file"))
                return log_debug_errno(SYNTHETIC_ERRNO(EINVAL),
                                       "Invalid hibernate type: %s", hibernate_location->swap->type);

        /* Only available in 4.17+ */
        if (hibernate_location->offset > 0 && access("/sys/power/resume_offset", W_OK) < 0) {
                if (errno == ENOENT) {
                        log_debug("Kernel too old, can't configure resume_offset for %s, ignoring: %" PRIu64,
                                  hibernate_location->swap->device, hibernate_location->offset);
                        return 0;
                }

                return log_debug_errno(errno, "/sys/power/resume_offset not writable: %m");
        }

        xsprintf(offset_str, "%" PRIu64, hibernate_location->offset);
        r = write_string_file("/sys/power/resume_offset", offset_str, WRITE_STRING_FILE_DISABLE_BUFFER);
        if (r < 0)
                return log_debug_errno(r, "Failed to write swap file offset to /sys/power/resume_offset for '%s': '%s': %m",
                                       hibernate_location->swap->device, offset_str);

        log_debug("Wrote resume_offset= value for %s to /sys/power/resume_offset: %s", hibernate_location->swap->device, offset_str);

        return 0;
}

#if 0 /// elogind uses a special variant to heed suspension modes
static int write_mode(char **modes) {
        int r = 0;
        char **mode;

        STRV_FOREACH(mode, modes) {
                int k;

                k = write_string_file("/sys/power/disk", *mode, WRITE_STRING_FILE_DISABLE_BUFFER);
                if (k >= 0)
                        return 0;

                log_debug_errno(k, "Failed to write '%s' to /sys/power/disk: %m", *mode);
                if (r >= 0)
                        r = k;
        }

        return r;
}
#else // 0
static int write_mode(SleepOperation operation, char **modes) {
        int r = 0;
        char **mode;
        static char const mode_disk[] = "/sys/power/disk";
        static char const mode_mem[] = "/sys/power/mem_sleep";
        char const* mode_location = SLEEP_SUSPEND == operation ? mode_mem : mode_disk;

        // If this is a supend, write that it is to mode_disk.
        if (operation == SLEEP_SUSPEND) {
                log_debug_elogind("Writing '%s' to '%s' ...", "suspend", mode_disk);
                (void) write_string_file(mode_disk, "suspend", WRITE_STRING_FILE_DISABLE_BUFFER);
        }

        // Now get the real action mode right:
        STRV_FOREACH(mode, modes) {
                int k;

                log_debug_elogind("Writing '%s' to '%s' ...", *mode, mode_location);
                k = write_string_file(mode_location, *mode, WRITE_STRING_FILE_DISABLE_BUFFER);
                if (k >= 0)
                        return 0;

                log_debug_errno(k, "Failed to write '%s' to %s: %m", *mode, mode_location);
                if (r >= 0)
                        r = k;
        }

        return r;
}
#endif // 0


static int write_state(FILE **f, char **states) {
        char **state;
        int r = 0;

        assert(f);
        assert(*f);

        STRV_FOREACH(state, states) {
                int k;

                k = write_string_stream(*f, *state, WRITE_STRING_FILE_DISABLE_BUFFER);
                if (k >= 0)
                        return 0;
                log_debug_errno(k, "Failed to write '%s' to /sys/power/state: %m", *state);
                if (r >= 0)
                        r = k;

                fclose(*f);
                *f = fopen("/sys/power/state", "we");
                if (!*f)
                        return -errno;
        }

        return r;
}

#if 0 /// elogind does neither ship homed nor supports any substitution right now
static int lock_all_homes(void) {
        _cleanup_(sd_bus_error_free) sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_(sd_bus_message_unrefp) sd_bus_message *m = NULL;
        _cleanup_(sd_bus_flush_close_unrefp) sd_bus *bus = NULL;
        int r;

        /* Let's synchronously lock all home directories managed by homed that have been marked for it. This
         * way the key material required to access these volumes is hopefully removed from memory. */

        r = sd_bus_open_system(&bus);
        if (r < 0)
                return log_warning_errno(r, "Failed to connect to system bus, ignoring: %m");

        r = sd_bus_message_new_method_call(
                        bus,
                        &m,
                        "org.freedesktop.home1",
                        "/org/freedesktop/home1",
                        "org.freedesktop.home1.Manager",
                        "LockAllHomes");
        if (r < 0)
                return bus_log_create_error(r);

        /* If homed is not running it can't have any home directories active either. */
        r = sd_bus_message_set_auto_start(m, false);
        if (r < 0)
                return log_error_errno(r, "Failed to disable auto-start of LockAllHomes() message: %m");

        r = sd_bus_call(bus, m, DEFAULT_TIMEOUT_USEC, &error, NULL);
        if (r < 0) {
                if (!bus_error_is_unknown_service(&error))
                        return log_error_errno(r, "Failed to lock home directories: %s", bus_error_message(&error, r));

                log_debug("systemd-homed is not running, locking of home directories skipped.");
        } else
                log_debug("Successfully requested locking of all home directories.");
        return 0;
}
#endif // 0

static int execute(
                const SleepConfig *sleep_config,
                SleepOperation operation,
                const char *action) {

        char *arguments[] = {
                NULL,
                (char*) "pre",
                /* NB: we use 'arg_operation' instead of 'operation' here, as we want to communicate the overall
                 * operation here, not the specific one, in case of s2h. */
                (char*) sleep_operation_to_string(arg_operation),
                NULL
        };
        static const char* const dirs[] = {
                SYSTEM_SLEEP_PATH,
#if 1 /// elogind also supports a hook dir in etc
                PKGSYSCONFDIR "/system-sleep",
#endif // 1
                NULL
        };
#if 1 /// elogind has to check hooks itself and tries to work around a missing nvidia-suspend script (if needed)
        Manager* m = (Manager*)sleep_config; // sleep-config.h has created the alias. We use 'm' internally to reduce confusion.
        void* gather_args[] = {
                [STDOUT_GENERATE] = m,
                [STDOUT_COLLECT] = m,
                [STDOUT_CONSUME] = m,
        };
        int have_nvidia = 0;
        unsigned vtnr = 0;
        int e;
        _cleanup_free_ char *l = NULL;

        log_debug_elogind("Called for '%s' (Manager is %s)", sleep_operation_to_string(operation), m ? "Set" : "NULL");
#endif // 1

        _cleanup_(hibernate_location_freep) HibernateLocation *hibernate_location = NULL;
        _cleanup_fclose_ FILE *f = NULL;
        char **modes, **states;
        int r;

        assert(sleep_config);
        assert(operation >= 0);
        assert(operation < _SLEEP_OPERATION_MAX);
        assert(operation != SLEEP_SUSPEND_THEN_HIBERNATE); /* Handled by execute_s2h() instead */

        states = sleep_config->states[operation];
        modes = sleep_config->modes[operation];

        if (strv_isempty(states))
                return log_error_errno(SYNTHETIC_ERRNO(EINVAL),
                                       "No sleep states configured for sleep operation %s, can't sleep.",
                                       sleep_operation_to_string(operation));

        /* This file is opened first, so that if we hit an error,
         * we can abort before modifying any state. */
        f = fopen("/sys/power/state", "we");
        if (!f)
                return log_error_errno(errno, "Failed to open /sys/power/state: %m");

#ifdef __GLIBC__ /// elogind must not disable buffers on musl-libc based systems
        setvbuf(f, NULL, _IONBF, 0);
#endif // __GLIBC__

        /* Configure hibernation settings if we are supposed to hibernate */
#if 0 /// elogind supports suspend modes, and keeps its config, so checking modes for emptyness doesn't cut it
        if (!strv_isempty(modes)) {
#else // 0
        if (operation != SLEEP_SUSPEND) {
#endif // 0
                r = find_hibernate_location(&hibernate_location);
                if (r < 0)
                        return log_error_errno(r, "Failed to find location to hibernate to: %m");
                if (r == 0) { /* 0 means: no hibernation location was configured in the kernel so far, let's
                               * do it ourselves then. > 0 means: kernel already had a configured hibernation
                               * location which we shouldn't touch. */
                        r = write_hibernate_location_info(hibernate_location);
                        if (r < 0)
                                return log_error_errno(r, "Failed to prepare for hibernation: %m");
                }
#if 0 /// elogind supports suspend modes
                r = write_mode(modes);
                if (r < 0)
                        return log_error_errno(r, "Failed to write mode to /sys/power/disk: %m");;
        }
#else // 0
        }
        r = write_mode(operation, modes);
        if (r < 0)
                return log_error_errno(r, "Failed to write mode to /sys/power/%s: %m",
                                       SLEEP_SUSPEND == operation ? "mem_sleep" : "disk");
#endif // 0

        /* Pass an action string to the call-outs. This is mostly our operation string, except if the
         * hibernate step of s-t-h fails, in which case we communicate that with a separate action. */
        if (!action)
                action = sleep_operation_to_string(operation);

        r = setenv("SYSTEMD_SLEEP_ACTION", action, 1);
        if (r != 0)
                log_warning_errno(errno, "Error setting SYSTEMD_SLEEP_ACTION=%s, ignoring: %m", action);

#if 0 /// elogind allows admins to configure that hook scripts must succeed. The systemd default does not cut it here
        (void) execute_directories(dirs, DEFAULT_TIMEOUT_USEC, NULL, NULL, arguments, NULL, EXEC_DIR_PARALLEL | EXEC_DIR_IGNORE_ERRORS);
        (void) lock_all_homes();
#else // 0
        m->callback_failed = false;
        m->callback_must_succeed = m->allow_suspend_interrupts;

        log_debug_elogind("Executing suspend hook scripts... (Must succeed: %s)",
                          m->callback_must_succeed ? "YES" : "no");

        r = execute_directories(dirs, DEFAULT_TIMEOUT_USEC, gather_output, gather_args, arguments, NULL, EXEC_DIR_NONE);

        log_debug_elogind("Result is %d (callback_failed: %s)", r, m->callback_failed ? "true" : "false");

        if (m->callback_must_succeed && (r || m->callback_failed)) {
                e = asprintf(&l, "A sleep script in %s or %s failed! [%d]\nThe system %s has been cancelled!",
                             SYSTEM_SLEEP_PATH, PKGSYSCONFDIR "/system-sleep", r, sleep_operation_to_string(operation));
                if (e < 0) {
                        log_oom();
                        return -ENOMEM;
                }

                if ( m->broadcast_suspend_interrupts )
                        utmp_wall(l, "root", "n/a", logind_wall_tty_filter, m);

                log_struct_errno(LOG_ERR, r, "MESSAGE_ID=" SD_MESSAGE_SLEEP_STOP_STR, LOG_MESSAGE("%s", l), "SLEEP=%s",
                                 sleep_operation_to_string(operation));

                return -ECANCELED;
        }
#endif // 0

        log_struct(LOG_INFO,
                   "MESSAGE_ID=" SD_MESSAGE_SLEEP_START_STR,
                   LOG_MESSAGE("Entering sleep state '%s'...", sleep_operation_to_string(operation)),
                   "SLEEP=%s", sleep_operation_to_string(arg_operation));

#if 1 /// elogind may try to send a suspend signal to an nvidia card
        if ( m->handle_nvidia_sleep )
                have_nvidia = nvidia_sleep(m, operation, &vtnr);
#endif // 1

        r = write_state(&f, states);
        if (r < 0)
                log_struct_errno(LOG_ERR, r,
                                 "MESSAGE_ID=" SD_MESSAGE_SLEEP_STOP_STR,
                                 LOG_MESSAGE("Failed to put system to sleep. System resumed again: %m"),
                                 "SLEEP=%s", sleep_operation_to_string(arg_operation));
        else
                log_struct(LOG_INFO,
                           "MESSAGE_ID=" SD_MESSAGE_SLEEP_STOP_STR,
                           LOG_MESSAGE("System returned from sleep state."),
                           "SLEEP=%s", sleep_operation_to_string(arg_operation));

#if 1 /// if put to sleep, elogind also has to wakeup an nvidia card
        if (have_nvidia)
                nvidia_sleep(m, _SLEEP_OPERATION_MAX, &vtnr);
#endif // 1

        arguments[1] = (char*) "post";
#if 0 /// elogind does not execute wakeup hook scripts in parallel, they might be order relevant
        (void) execute_directories(dirs, DEFAULT_TIMEOUT_USEC, NULL, NULL, arguments, NULL, EXEC_DIR_PARALLEL | EXEC_DIR_IGNORE_ERRORS);
#else // 0
        (void) execute_directories(dirs, DEFAULT_TIMEOUT_USEC, NULL, NULL, arguments, NULL, EXEC_DIR_IGNORE_ERRORS);
#endif // 0

        return r;
}

static int execute_s2h(const SleepConfig *sleep_config) {
        _cleanup_close_ int tfd = -1;
        char buf[FORMAT_TIMESPAN_MAX];
        struct itimerspec ts = {};
        int r;

        assert(sleep_config);

        tfd = timerfd_create(CLOCK_BOOTTIME_ALARM, TFD_NONBLOCK|TFD_CLOEXEC);
        if (tfd < 0)
                return log_error_errno(errno, "Error creating timerfd: %m");

        log_debug("Set timerfd wake alarm for %s",
                  format_timespan(buf, sizeof(buf), sleep_config->hibernate_delay_sec, USEC_PER_SEC));

        timespec_store(&ts.it_value, sleep_config->hibernate_delay_sec);

        r = timerfd_settime(tfd, 0, &ts, NULL);
        if (r < 0)
                return log_error_errno(errno, "Error setting hibernate timer: %m");

        r = execute(sleep_config, SLEEP_SUSPEND, NULL);
        if (r < 0)
                return r;

        r = fd_wait_for_event(tfd, POLLIN, 0);
        if (r < 0)
                return log_error_errno(r, "Error polling timerfd: %m");
        if (!FLAGS_SET(r, POLLIN)) /* We woke up before the alarm time, we are done. */
                return 0;

        tfd = safe_close(tfd);

        /* If woken up after alarm time, hibernate */
        log_debug("Attempting to hibernate after waking from %s timer",
                  format_timespan(buf, sizeof(buf), sleep_config->hibernate_delay_sec, USEC_PER_SEC));

        r = execute(sleep_config, SLEEP_HIBERNATE, NULL);
        if (r < 0) {
                log_notice("Couldn't hibernate, will try to suspend again.");

                r = execute(sleep_config, SLEEP_SUSPEND, "suspend-after-failed-hibernate");
                if (r < 0)
                        return r;
        }

        return 0;
}

#if 0 /// elogind calls execute() by itself and does not need another binary
static int help(void) {
        _cleanup_free_ char *link = NULL;
        int r;

        r = terminal_urlify_man("systemd-suspend.service", "8", &link);
        if (r < 0)
                return log_oom();

        printf("%s COMMAND\n\n"
               "Suspend the system, hibernate the system, or both.\n\n"
               "  -h --help              Show this help and exit\n"
               "  --version              Print version string and exit\n"
               "\nCommands:\n"
               "  suspend                Suspend the system\n"
               "  hibernate              Hibernate the system\n"
               "  hybrid-sleep           Both hibernate and suspend the system\n"
               "  suspend-then-hibernate Initially suspend and then hibernate\n"
               "                         the system after a fixed period of time\n"
               "\nSee the %s for details.\n",
               program_invocation_short_name,
               link);

        return 0;
}

static int parse_argv(int argc, char *argv[]) {
        enum {
                ARG_VERSION = 0x100,
        };

        static const struct option options[] = {
                { "help",         no_argument,       NULL, 'h'           },
                { "version",      no_argument,       NULL, ARG_VERSION   },
                {}
        };

        int c;

        assert(argc >= 0);
        assert(argv);

        while ((c = getopt_long(argc, argv, "h", options, NULL)) >= 0)
                switch(c) {
                case 'h':
                        return help();

                case ARG_VERSION:
                        return version();

                case '?':
                        return -EINVAL;

                default:
                        assert_not_reached("Unhandled option");
                }

        if (argc - optind != 1)
                return log_error_errno(SYNTHETIC_ERRNO(EINVAL),
                                       "Usage: %s COMMAND",
                                       program_invocation_short_name);

        arg_operation = sleep_operation_from_string(argv[optind]);
        if (arg_operation < 0)
                return log_error_errno(SYNTHETIC_ERRNO(EINVAL), "Unknown command '%s'.", argv[optind]);

        return 1 /* work to do */;
}

static int run(int argc, char *argv[]) {
        _cleanup_(free_sleep_configp) SleepConfig *sleep_config = NULL;
        int r;

        log_setup();

        r = parse_argv(argc, argv);
        if (r <= 0)
                return r;

        r = parse_sleep_config(&sleep_config);
        if (r < 0)
                return r;

        if (!sleep_config->allow[arg_operation])
                return log_error_errno(SYNTHETIC_ERRNO(EACCES),
                                       "Sleep operation \"%s\" is disabled by configuration, refusing.",
                                       sleep_operation_to_string(arg_operation));

        switch (arg_operation) {

        case SLEEP_SUSPEND_THEN_HIBERNATE:
                r = execute_s2h(sleep_config);
                break;

        case SLEEP_HYBRID_SLEEP:
                r = execute(sleep_config, SLEEP_HYBRID_SLEEP, NULL);
                if (r < 0) {
                        /* If we can't hybrid sleep, then let's try to suspend at least. After all, the user
                         * asked us to do both: suspend + hibernate, and it's almost certainly the
                         * hibernation that failed, hence still do the other thing, the suspend. */

                        log_notice("Couldn't hybrid sleep, will try to suspend instead.");

                        r = execute(sleep_config, SLEEP_SUSPEND, "suspend-after-failed-hybrid-sleep");
                }

                break;

        default:
                r = execute(sleep_config, arg_operation, NULL);
                break;
        }

        return r;
}

DEFINE_MAIN_FUNCTION(run);
#else // 0

int do_sleep(Manager *m, SleepOperation operation) {
        int r;

        assert(operation < _SLEEP_OPERATION_MAX);
        assert(m);

        arg_operation = operation;

        log_debug_elogind("Called for '%s'", sleep_operation_to_string(operation));

        /* Re-load the sleep configuration, so users can change their options
         * on-the-fly without having to reload elogind
         */
        r = parse_sleep_config(&m);
        if (r < 0)
                return r;

        if (!m->allow[arg_operation])
                return log_error_errno(SYNTHETIC_ERRNO(EACCES),
                                       "Sleep operation \"%s\" is disabled by configuration, refusing.",
                                       sleep_operation_to_string(arg_operation));

        switch (arg_operation) {

        case SLEEP_SUSPEND_THEN_HIBERNATE:
                r = execute_s2h(m);
                break;

        case SLEEP_HYBRID_SLEEP:
                r = execute(m, SLEEP_HYBRID_SLEEP, NULL);
                if (r < 0) {
                        /* If we can't hybrid sleep, then let's try to suspend at least. After all, the user
                         * asked us to do both: suspend + hibernate, and it's almost certainly the
                         * hibernation that failed, hence still do the other thing, the suspend. */

                        log_notice("Couldn't hybrid sleep, will try to suspend instead.");

                        r = execute(m, SLEEP_SUSPEND, "suspend-after-failed-hybrid-sleep");
                }

                break;

        default:
                r = execute(m, arg_operation, NULL);
                break;
        }

        return r;
}

#endif // 0
