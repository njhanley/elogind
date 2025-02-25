/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include <fcntl.h>
#include <linux/magic.h>
#include <sched.h>
#include <unistd.h>

#include "alloc-util.h"
#include "errno-list.h"
#include "fd-util.h"
#include "macro.h"
#include "mountpoint-util.h"
#include "namespace-util.h"
#include "path-util.h"
#include "stat-util.h"
#include "tests.h"
#include "tmpfile-util.h"

static void test_files_same(void) {
        _cleanup_close_ int fd = -1;
        char name[] = "/tmp/test-files_same.XXXXXX";
        char name_alias[] = "/tmp/test-files_same.alias";

        log_info("/* %s */", __func__);

        fd = mkostemp_safe(name);
        assert_se(fd >= 0);
        assert_se(symlink(name, name_alias) >= 0);

        assert_se(files_same(name, name, 0));
        assert_se(files_same(name, name, AT_SYMLINK_NOFOLLOW));
        assert_se(files_same(name, name_alias, 0));
        assert_se(!files_same(name, name_alias, AT_SYMLINK_NOFOLLOW));

        unlink(name);
        unlink(name_alias);
}

#if 0 /// UNNEEDED by elogind
static void test_is_symlink(void) {
        char name[] = "/tmp/test-is_symlink.XXXXXX";
        char name_link[] = "/tmp/test-is_symlink.link";
        _cleanup_close_ int fd = -1;

        log_info("/* %s */", __func__);

        fd = mkostemp_safe(name);
        assert_se(fd >= 0);
        assert_se(symlink(name, name_link) >= 0);

        assert_se(is_symlink(name) == 0);
        assert_se(is_symlink(name_link) == 1);
        assert_se(is_symlink("/a/file/which/does/not/exist/i/guess") < 0);

        unlink(name);
        unlink(name_link);
}

static void test_path_is_fs_type(void) {
        log_info("/* %s */", __func__);

        /* run might not be a mount point in build chroots */
        if (path_is_mount_point("/run", NULL, AT_SYMLINK_FOLLOW) > 0) {
                assert_se(path_is_fs_type("/run", TMPFS_MAGIC) > 0);
                assert_se(path_is_fs_type("/run", BTRFS_SUPER_MAGIC) == 0);
        }
        if (path_is_mount_point("/proc", NULL, AT_SYMLINK_FOLLOW) > 0) {
                assert_se(path_is_fs_type("/proc", PROC_SUPER_MAGIC) > 0);
                assert_se(path_is_fs_type("/proc", BTRFS_SUPER_MAGIC) == 0);
        }
        assert_se(path_is_fs_type("/i-dont-exist", BTRFS_SUPER_MAGIC) == -ENOENT);
}

static void test_path_is_temporary_fs(void) {
        const char *s;
        int r;

        log_info("/* %s */", __func__);

        FOREACH_STRING(s, "/", "/run", "/sys", "/sys/", "/proc", "/i-dont-exist", "/var", "/var/lib") {
                r = path_is_temporary_fs(s);

                log_info_errno(r, "path_is_temporary_fs(\"%s\"): %d, %s",
                               s, r, r < 0 ? errno_to_name(r) : yes_no(r));
        }

        /* run might not be a mount point in build chroots */
        if (path_is_mount_point("/run", NULL, AT_SYMLINK_FOLLOW) > 0)
                assert_se(path_is_temporary_fs("/run") > 0);
        assert_se(path_is_temporary_fs("/proc") == 0);
        assert_se(path_is_temporary_fs("/i-dont-exist") == -ENOENT);
}

static void test_path_is_read_only_fs(void) {
        const char *s;
        int r;

        log_info("/* %s */", __func__);

        FOREACH_STRING(s, "/", "/run", "/sys", "/sys/", "/proc", "/i-dont-exist", "/var", "/var/lib") {
                r = path_is_read_only_fs(s);

                log_info_errno(r, "path_is_read_only_fs(\"%s\"): %d, %s",
                               s, r, r < 0 ? errno_to_name(r) : yes_no(r));
        }

        if (path_is_mount_point("/sys", NULL, AT_SYMLINK_FOLLOW) > 0)
                assert_se(IN_SET(path_is_read_only_fs("/sys"), 0, 1));

        assert_se(path_is_read_only_fs("/proc") == 0);
        assert_se(path_is_read_only_fs("/i-dont-exist") == -ENOENT);
}

static void test_fd_is_ns(void) {
        _cleanup_close_ int fd = -1;

        log_info("/* %s */", __func__);

        assert_se(fd_is_ns(STDIN_FILENO, CLONE_NEWNET) == 0);
        assert_se(fd_is_ns(STDERR_FILENO, CLONE_NEWNET) == 0);
        assert_se(fd_is_ns(STDOUT_FILENO, CLONE_NEWNET) == 0);

        fd = open("/proc/self/ns/mnt", O_CLOEXEC|O_RDONLY);
        if (fd < 0) {
                assert_se(errno == ENOENT);
                log_notice("Path %s not found, skipping test", "/proc/self/ns/mnt");
                return;
        }
        assert_se(fd >= 0);
        assert_se(IN_SET(fd_is_ns(fd, CLONE_NEWNET), 0, -EUCLEAN));
        fd = safe_close(fd);

        assert_se((fd = open("/proc/self/ns/ipc", O_CLOEXEC|O_RDONLY)) >= 0);
        assert_se(IN_SET(fd_is_ns(fd, CLONE_NEWIPC), 1, -EUCLEAN));
        fd = safe_close(fd);

        assert_se((fd = open("/proc/self/ns/net", O_CLOEXEC|O_RDONLY)) >= 0);
        assert_se(IN_SET(fd_is_ns(fd, CLONE_NEWNET), 1, -EUCLEAN));
}
#endif // 0

static void test_device_major_minor_valid(void) {
        log_info("/* %s */", __func__);

        /* on glibc dev_t is 64bit, even though in the kernel it is only 32bit */
        assert_cc(sizeof(dev_t) == sizeof(uint64_t));

        assert_se(DEVICE_MAJOR_VALID(0U));
        assert_se(DEVICE_MINOR_VALID(0U));

        assert_se(DEVICE_MAJOR_VALID(1U));
        assert_se(DEVICE_MINOR_VALID(1U));

        assert_se(!DEVICE_MAJOR_VALID(-1U));
        assert_se(!DEVICE_MINOR_VALID(-1U));

        assert_se(DEVICE_MAJOR_VALID(1U << 10));
        assert_se(DEVICE_MINOR_VALID(1U << 10));

        assert_se(DEVICE_MAJOR_VALID((1U << 12) - 1));
        assert_se(DEVICE_MINOR_VALID((1U << 20) - 1));

        assert_se(!DEVICE_MAJOR_VALID((1U << 12)));
        assert_se(!DEVICE_MINOR_VALID((1U << 20)));

        assert_se(!DEVICE_MAJOR_VALID(1U << 25));
        assert_se(!DEVICE_MINOR_VALID(1U << 25));

        assert_se(!DEVICE_MAJOR_VALID(UINT32_MAX));
        assert_se(!DEVICE_MINOR_VALID(UINT32_MAX));

        assert_se(!DEVICE_MAJOR_VALID(UINT64_MAX));
        assert_se(!DEVICE_MINOR_VALID(UINT64_MAX));

        assert_se(DEVICE_MAJOR_VALID(major(0)));
        assert_se(DEVICE_MINOR_VALID(minor(0)));
}

static void test_device_path_make_canonical_one(const char *path) {
        _cleanup_free_ char *resolved = NULL, *raw = NULL;
        struct stat st;
        dev_t devno;
        mode_t mode;
        int r;

        log_debug("> %s", path);

        if (stat(path, &st) < 0) {
                assert(errno == ENOENT);
                log_notice("Path %s not found, skipping test", path);
                return;
        }

        r = device_path_make_canonical(st.st_mode, st.st_rdev, &resolved);
        if (r == -ENOENT) {
                /* maybe /dev/char/x:y and /dev/block/x:y are missing in this test environment, because we
                 * run in a container or so? */
                log_notice("Device %s cannot be resolved, skipping test", path);
                return;
        }

        assert_se(r >= 0);
        assert_se(path_equal(path, resolved));

        assert_se(device_path_make_major_minor(st.st_mode, st.st_rdev, &raw) >= 0);
        assert_se(device_path_parse_major_minor(raw, &mode, &devno) >= 0);

        assert_se(st.st_rdev == devno);
        assert_se((st.st_mode & S_IFMT) == (mode & S_IFMT));
}

static void test_device_path_make_canonical(void) {
        log_info("/* %s */", __func__);

        test_device_path_make_canonical_one("/dev/null");
        test_device_path_make_canonical_one("/dev/zero");
        test_device_path_make_canonical_one("/dev/full");
        test_device_path_make_canonical_one("/dev/random");
        test_device_path_make_canonical_one("/dev/urandom");
        test_device_path_make_canonical_one("/dev/tty");

#if 0 /// UNNEEDED by elogind
        if (is_device_node("/run/systemd/inaccessible/blk") > 0) {
                test_device_path_make_canonical_one("/run/systemd/inaccessible/chr");
                test_device_path_make_canonical_one("/run/systemd/inaccessible/blk");
        }
#endif // 0
}

int main(int argc, char *argv[]) {
        log_show_color(true);
        test_setup_logging(LOG_INFO);

        test_files_same();
#if 0 /// UNNEEDED by elogind
        test_is_symlink();
        test_path_is_fs_type();
        test_path_is_temporary_fs();
        test_path_is_read_only_fs();
        test_fd_is_ns();
#endif // 0
        test_device_major_minor_valid();
        test_device_path_make_canonical();

        return 0;
}
