/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <alloca.h>
#include <stdbool.h>
#include <stddef.h>

#include "macro.h"
#include "string-util.h"
#include "strv.h"
#include "time-util.h"

#define PATH_SPLIT_SBIN_BIN(x) x "sbin:" x "bin"
#define PATH_SPLIT_SBIN_BIN_NULSTR(x) x "sbin\0" x "bin\0"

#define PATH_NORMAL_SBIN_BIN(x) x "bin"
#define PATH_NORMAL_SBIN_BIN_NULSTR(x) x "bin\0"

#if HAVE_SPLIT_BIN
#  define PATH_SBIN_BIN(x) PATH_SPLIT_SBIN_BIN(x)
#  define PATH_SBIN_BIN_NULSTR(x) PATH_SPLIT_SBIN_BIN_NULSTR(x)
#else
#  define PATH_SBIN_BIN(x) PATH_NORMAL_SBIN_BIN(x)
#  define PATH_SBIN_BIN_NULSTR(x) PATH_NORMAL_SBIN_BIN_NULSTR(x)
#endif

#define DEFAULT_PATH_NORMAL PATH_SBIN_BIN("/usr/local/") ":" PATH_SBIN_BIN("/usr/")
#define DEFAULT_PATH_NORMAL_NULSTR PATH_SBIN_BIN_NULSTR("/usr/local/") PATH_SBIN_BIN_NULSTR("/usr/")
#define DEFAULT_PATH_SPLIT_USR DEFAULT_PATH_NORMAL ":" PATH_SBIN_BIN("/")
#define DEFAULT_PATH_SPLIT_USR_NULSTR DEFAULT_PATH_NORMAL_NULSTR PATH_SBIN_BIN_NULSTR("/")
#define DEFAULT_PATH_COMPAT PATH_SPLIT_SBIN_BIN("/usr/local/") ":" PATH_SPLIT_SBIN_BIN("/usr/") ":" PATH_SPLIT_SBIN_BIN("/")

#if HAVE_SPLIT_USR
#  define DEFAULT_PATH DEFAULT_PATH_SPLIT_USR
#  define DEFAULT_PATH_NULSTR DEFAULT_PATH_SPLIT_USR_NULSTR
#else
#  define DEFAULT_PATH DEFAULT_PATH_NORMAL
#  define DEFAULT_PATH_NULSTR DEFAULT_PATH_NORMAL_NULSTR
#endif

#ifndef DEFAULT_USER_PATH
#  define DEFAULT_USER_PATH DEFAULT_PATH
#endif

static inline bool is_path(const char *p) {
        assert(p);
        return strchr(p, '/');
}

static inline bool path_is_absolute(const char *p) {
        assert(p);
        return p[0] == '/';
}

#if 0 /// UNNEEDED by elogind
int path_split_and_make_absolute(const char *p, char ***ret);
char* path_make_absolute(const char *p, const char *prefix);
#endif // 0
int safe_getcwd(char **ret);
int path_make_absolute_cwd(const char *p, char **ret);
#if 0 /// UNNEEDED by elogind
#endif // 0
int path_make_relative(const char *from, const char *to, char **ret);
char *path_startswith_full(const char *path, const char *prefix, bool accept_dot_dot) _pure_;
static inline char* path_startswith(const char *path, const char *prefix) {
        return path_startswith_full(path, prefix, true);
}
int path_compare(const char *a, const char *b) _pure_;

static inline bool path_equal(const char *a, const char *b) {
        return path_compare(a, b) == 0;
}

bool path_equal_or_files_same(const char *a, const char *b, int flags);
/* Compares only the last portion of the input paths, ie: the filenames */
bool path_equal_filename(const char *a, const char *b);

char* path_extend_internal(char **x, ...);
#define path_extend(x, ...) path_extend_internal(x, __VA_ARGS__, POINTER_MAX)
#define path_join(...) path_extend_internal(NULL, __VA_ARGS__, POINTER_MAX)

char* path_simplify(char *path);

enum {
        PATH_CHECK_FATAL    = 1 << 0,  /* If not set, then error message is appended with 'ignoring'. */
        PATH_CHECK_ABSOLUTE = 1 << 1,
        PATH_CHECK_RELATIVE = 1 << 2,
};

int path_simplify_and_warn(char *path, unsigned flag, const char *unit, const char *filename, unsigned line, const char *lvalue);

static inline bool path_equal_ptr(const char *a, const char *b) {
        return !!a == !!b && (!a || path_equal(a, b));
}

/* Note: the search terminates on the first NULL item. */
#define PATH_IN_SET(p, ...) path_strv_contains(STRV_MAKE(__VA_ARGS__), p)

char* path_startswith_strv(const char *p, char **set);
#define PATH_STARTSWITH_SET(p, ...) path_startswith_strv(p, STRV_MAKE(__VA_ARGS__))

#if 0 /// UNNEEDED by elogind
int path_strv_make_absolute_cwd(char **l);
#endif // 0
char** path_strv_resolve(char **l, const char *root);
char** path_strv_resolve_uniq(char **l, const char *root);

#if 0 /// UNNEEDED by elogind
int find_executable_full(const char *name, bool use_path_envvar, char **ret_filename, int *ret_fd);
static inline int find_executable(const char *name, char **ret_filename) {
        return find_executable_full(name, true, ret_filename, NULL);
}

bool paths_check_timestamp(const char* const* paths, usec_t *paths_ts_usec, bool update);

int fsck_exists(const char *fstype);
#endif // 0

/* Iterates through the path prefixes of the specified path, going up
 * the tree, to root. Also returns "" (and not "/"!) for the root
 * directory. Excludes the specified directory itself */
#define PATH_FOREACH_PREFIX(prefix, path)                               \
        for (char *_slash = ({                                          \
                                path_simplify(strcpy(prefix, path));    \
                                streq(prefix, "/") ? NULL : strrchr(prefix, '/'); \
                        });                                             \
             _slash && ((*_slash = 0), true);                           \
             _slash = strrchr((prefix), '/'))

/* Same as PATH_FOREACH_PREFIX but also includes the specified path itself */
#define PATH_FOREACH_PREFIX_MORE(prefix, path)                          \
        for (char *_slash = ({                                          \
                                path_simplify(strcpy(prefix, path));    \
                                if (streq(prefix, "/"))                 \
                                        prefix[0] = 0;                  \
                                strrchr(prefix, 0);                     \
                        });                                             \
             _slash && ((*_slash = 0), true);                           \
             _slash = strrchr((prefix), '/'))

/* Similar to path_join(), but only works for two components, and only the first one may be NULL and returns
 * an alloca() buffer, or possibly a const pointer into the path parameter. */
#define prefix_roota(root, path)                                        \
        ({                                                              \
                const char* _path = (path), *_root = (root), *_ret;     \
                char *_p, *_n;                                          \
                size_t _l;                                              \
                while (_path[0] == '/' && _path[1] == '/')              \
                        _path ++;                                       \
                if (isempty(_root))                                     \
                        _ret = _path;                                   \
                else {                                                  \
                        _l = strlen(_root) + 1 + strlen(_path) + 1;     \
                        _n = newa(char, _l);                            \
                        _p = stpcpy(_n, _root);                         \
                        while (_p > _n && _p[-1] == '/')                \
                                _p--;                                   \
                        if (_path[0] != '/')                            \
                                *(_p++) = '/';                          \
                        strcpy(_p, _path);                              \
                        _ret = _n;                                      \
                }                                                       \
                _ret;                                                   \
        })

#if 0 /// UNNEEDED by elogind
#endif // 0

char* dirname_malloc(const char *path);
int path_find_first_component(const char **p, bool accept_dot_dot, const char **ret);
int path_find_last_component(const char *path, bool accept_dot_dot, const char **next, const char **ret);
const char *last_path_component(const char *path);
int path_extract_filename(const char *path, char **ret);
int path_extract_directory(const char *path, char **ret);

bool filename_is_valid(const char *p) _pure_;
bool path_is_valid_full(const char *p, bool accept_dot_dot) _pure_;
static inline bool path_is_valid(const char *p) {
        return path_is_valid_full(p, true);
}
static inline bool path_is_safe(const char *p) {
        return path_is_valid_full(p, false);
}
bool path_is_normalized(const char *p) _pure_;

char *file_in_same_dir(const char *path, const char *filename);

bool hidden_or_backup_file(const char *filename) _pure_;

#if 0 /// UNNEEDED by elogind
bool is_device_path(const char *path);

bool valid_device_node_path(const char *path);
bool valid_device_allow_pattern(const char *path);

int systemd_installation_has_version(const char *root, unsigned minimal_version);
#endif // 0

bool dot_or_dot_dot(const char *path);

static inline const char *skip_dev_prefix(const char *p) {
        const char *e;

        /* Drop any /dev prefix if there is any */

        e = path_startswith(p, "/dev/");

        return e ?: p;
}

bool empty_or_root(const char *path);
static inline const char *empty_to_root(const char *path) {
        return isempty(path) ? "/" : path;
}

bool path_strv_contains(char **l, const char *path);
#if 0 /// UNNEEDED by elogind
bool prefixed_path_strv_contains(char **l, const char *path);
#endif // 0
