SUFFIXES= .rst

EXTRA_DIST = COPYING scripts/findstatic.pl swupd.bash

AM_CFLAGS = -fPIC -g -D_FORTIFY_SOURCE=2 -fno-common -std=gnu99 \
	-O$(OPTIMIZE) \
	-Wall \
	-Wextra \
	-Wformat-security \
	-Wlogical-op \
	-Wunreachable-code \
	-Wswitch-default \
	-Wcast-align \
	-Wbad-function-cast \
	-Winline \
	-Wundef \
	-Wnested-externs \
	-Wredundant-decls

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = swupd

AM_CPPFLAGS = $(AM_CFLAGS) $(libarchive_CFLAGS)

swupd_SOURCES = \
	src/alias.c \
	src/alias.h \
	src/archives.c \
	src/autoupdate.c \
	src/binary_loader.c \
	src/bundle.c \
	src/check_update.c \
	src/clean.c \
	src/clr_bundle_add.c \
	src/clr_bundle_ls.c \
	src/clr_bundle_rm.c \
	src/curl.c \
	src/curl_async.c \
	src/delta.c \
	src/extra_files.c \
	src/filedesc.c \
	src/fullfile.c \
	src/globals.c \
	src/hash.c \
	src/hashdump.c \
	src/helpers.c \
	src/heuristics.c \
	src/info.c \
	src/lib/hashmap.c \
	src/lib/hashmap.h \
	src/lib/list.c \
	src/lib/list.h \
	src/lib/log.c \
	src/lib/log.h \
	src/lib/macros.h \
	src/lib/strings.c \
	src/lib/strings.h \
	src/lib/sys.c \
	src/lib/sys.h \
	src/lib/thread_pool.c \
	src/lib/thread_pool.h \
	src/lock.c \
	src/main.c \
	src/manifest.c \
	src/mirror.c \
	src/packs.c \
	src/scripts.c \
	src/search.c \
	src/signature.c \
	src/signature.h \
	src/staging.c \
	src/stats.c \
	src/subscriptions.c \
	src/swupd.h \
	src/swupd_build_opts.h \
	src/swupd_build_variant.h \
	src/swupd_curl.h \
	src/swupd_curl_internal.h \
	src/swupd_exit_codes.h \
	src/swupd_internal.h \
	src/telemetry.c \
	src/timelist.c \
	src/timelist.h \
	src/update.c \
	src/verify.c \
	src/verifytime.c \
	src/version.c \
	src/xattrs.c \
	src/xattrs.h

swupd_LDADD = \
	$(lzma_LIBS) \
	$(zlib_LIBS) \
	$(bzip2_LIBS) \
	$(openssl_LIBS) \
	$(curl_LIBS) \
	$(bsdiff_LIBS) \
	$(libarchive_LIBS) \
	$(pthread_LIBS)

verifytime_SOURCES = src/verifytime.h \
	src/verifytime.c \
	src/verifytime_main.c

verifytime_LDADD =

bin_PROGRAMS += verifytime

EXTRA_DIST += \
	data/check-update.service \
	data/check-update.timer \
	data/swupd-update.service \
	data/swupd-update.timer \
	data/verifytime.service

DISTCHECK_CONFIGURE_FLAGS = \
	--with-systemdsystemunitdir=$$dc_install_base/$(systemdunitdir)

systemdunitdir = @SYSTEMD_UNITDIR@

systemdunit_DATA = \
	data/check-update.service \
	data/check-update.timer \
	data/swupd-update.service \
	data/swupd-update.timer \
	data/verifytime.service

TEST_EXTENSIONS = .bats .prereq .test

if ENABLE_TESTS
tap_driver = env AM_TAP_AWK='$(AWK)' $(SHELL) \
	$(top_srcdir)/tap-driver.sh

BATS_LOG_DRIVER = $(tap_driver)

TESTS = $(dist_check_SCRIPTS) $(UNIT_TESTS)

BATS = \
	test/functional/bundleadd/add-alias-basic.bats \
	test/functional/bundleadd/add-bad-hash.bats \
	test/functional/bundleadd/add-bad-hash-state.bats \
	test/functional/bundleadd/add-bad-manifest.bats \
	test/functional/bundleadd/add-basics.bats \
	test/functional/bundleadd/add-boot-file.bats \
	test/functional/bundleadd/add-boot-skip.bats \
	test/functional/bundleadd/add-client-certificate.bats \
	test/functional/bundleadd/add-directory.bats \
	test/functional/bundleadd/add-experimental.bats \
	test/functional/bundleadd/add-fall-back-to-fullfile.bats \
	test/functional/bundleadd/add-include.bats \
	test/functional/bundleadd/add-install-time.bats \
	test/functional/bundleadd/add-multiple.bats \
	test/functional/bundleadd/add-no-disk-space.bats \
	test/functional/bundleadd/add-no-signature.bats \
	test/functional/bundleadd/add-skip-scripts.bats \
	test/functional/bundleadd/add-uses-fullfile.bats \
	test/functional/bundleadd/add-uses-zeropack.bats \
	test/functional/bundleadd/add-verify-fix-path.bats \
	test/functional/bundlelist/list-client-certificate.bats \
	test/functional/bundlelist/list-all.bats \
	test/functional/bundlelist/list-deps-flat.bats \
	test/functional/bundlelist/list-deps-invalid-bundle.bats \
	test/functional/bundlelist/list-deps-nested.bats \
	test/functional/bundlelist/list-experimental.bats \
	test/functional/bundlelist/list-has-dep-nested.bats \
	test/functional/bundlelist/list-has-dep-nested-not-installed.bats \
	test/functional/bundlelist/list-has-dep-nested-server.bats \
	test/functional/bundlelist/list-installed.bats \
	test/functional/bundlelist/list-no-deps.bats \
	test/functional/bundlelist/list-no-disk-space.bats \
	test/functional/bundlelist/list-none-has-deps.bats \
	test/functional/bundleremove/remove-basics.bats \
	test/functional/bundleremove/remove-boot-file.bats \
	test/functional/bundleremove/remove-client-certificate.bats \
	test/functional/bundleremove/remove-include.bats \
	test/functional/bundleremove/remove-include-nested.bats \
	test/functional/bundleremove/remove-no-disk-space.bats \
	test/functional/bundleremove/remove-os-core.bats \
	test/functional/bundleremove/remove-parse-args.bats \
	test/functional/bundleremove/remove-with-dependency.bats \
	test/functional/checkupdate/chk-update-client-certificate.bats \
	test/functional/checkupdate/chk-update-new-version.bats \
	test/functional/checkupdate/chk-update-no-server-content.bats \
	test/functional/checkupdate/chk-update-no-target-content.bats \
	test/functional/checkupdate/chk-update-slow-server.bats \
	test/functional/checkupdate/chk-update-version-match.bats \
	test/functional/hashdump/hashdump-file-hash.bats \
	test/functional/mirror/mirror-createdir.bats \
	test/functional/mirror/mirror-createdir-negative.bats \
	test/functional/search/search-content-check-negative.bats \
	test/functional/search/search-content-check-positive.bats \
	test/functional/search/search-client-certificate.bats \
	test/functional/search/search-experimental.bats \
	test/functional/search/search-no-disk-space.bats \
	test/functional/search/search-sort.bats \
	test/functional/update/update-boot-file.bats \
	test/functional/update/update-boot-skip.bats \
	test/functional/update/update-bundle-removed.bats \
	test/functional/update/update-client-certificate.bats \
	test/functional/update/update-download.bats \
	test/functional/update/update-fail-to-get-mom.bats \
	test/functional/update/update-include.bats \
	test/functional/update/update-include-old-bundle.bats \
	test/functional/update/update-include-old-bundle-with-tracked-file.bats \
	test/functional/update/update-manifest-delta.bats \
	test/functional/update/update-minversion.bats \
	test/functional/update/update-missing-os-core.bats \
	test/functional/update/update-newest-added.bats \
	test/functional/update/update-newest-deleted.bats \
	test/functional/update/update-newest-ghosted.bats \
	test/functional/update/update-non-responsive.bats \
	test/functional/update/update-no-disk-space.bats \
	test/functional/update/update-older-server-version.bats \
	test/functional/update/update-rename.bats \
	test/functional/update/update-rename-ghosted.bats \
	test/functional/update/update-re-update-bad-os-release.bats \
	test/functional/update/update-re-update-required.bats \
	test/functional/update/update-show-time.bats \
	test/functional/update/update-skip-scripts.bats \
	test/functional/update/update-slow-server.bats \
	test/functional/update/update-statedir-bad-hash.bats \
	test/functional/update/update-status.bats \
	test/functional/update/update-status-no-server-content.bats \
	test/functional/update/update-status-no-target-content.bats \
	test/functional/update/update-use-full-file.bats \
	test/functional/update/update-use-pack.bats \
	test/functional/update/update-verify-fix-path-hash-mismatch.bats \
	test/functional/update/update-verify-fix-path-missing-dir.bats \
	test/functional/update/update-verify-fullfile-hash.bats \
	test/functional/update/update-with-mirror.bats \
	test/functional/update/update-with-old-mirror.bats \
	test/functional/update/update-with-slightly-old-mirror.bats \
	test/functional/usability/usa-completion-basic.bats \
	test/functional/usability/usa-download-retries.bats \
	test/functional/usability/usa-external-modules.bats \
	test/functional/verify/verify-add-missing-include.bats \
	test/functional/verify/verify-add-missing-include-old.bats \
	test/functional/verify/verify-boot-file.bats \
	test/functional/verify/verify-boot-file-deleted.bats \
	test/functional/verify/verify-boot-skip.bats \
	test/functional/verify/verify-bundle.bats \
	test/functional/verify/verify-client-certificate.bats \
	test/functional/verify/verify-directory-tree-deleted.bats \
	test/functional/verify/verify-empty-dir-deleted.bats \
	test/functional/verify/verify-fix.bats \
	test/functional/verify/verify-fix-unsafe-to-delete.bats \
	test/functional/verify/verify-fix-version-mismatch.bats \
	test/functional/verify/verify-fix-version-mismatch-override.bats \
	test/functional/verify/verify-format-mismatch.bats \
	test/functional/verify/verify-format-mismatch-override.bats \
	test/functional/verify/verify-ghosted.bats \
	test/functional/verify/verify-good.bats \
	test/functional/verify/verify-install-bad.bats \
	test/functional/verify/verify-install.bats \
	test/functional/verify/verify-install-directory.bats \
	test/functional/verify/verify-install-latest-directory.bats \
	test/functional/verify/verify-install-multiple.bats \
	test/functional/verify/verify-install-no-fullfile-fallbacks.bats \
	test/functional/verify/verify-install-no-zero-packs.bats \
	test/functional/verify/verify-latest-missing.bats \
	test/functional/verify/verify-missing-file.bats \
	test/functional/verify/verify-no-disk-space.bats \
	test/functional/verify/verify-picky.bats \
	test/functional/verify/verify-picky-downgrade.bats \
	test/functional/verify/verify-picky-ghosted.bats \
	test/functional/verify/verify-picky-ghosted-missing.bats \
	test/functional/verify/verify-picky-whitelist.bats \
	test/functional/verify/verify-skip-scripts.bats

UNIT_TESTS = \
	test/unit/test_strings.test

dist_check_SCRIPTS = $(BATS)
# Must be run before all other tests
dist_check_SCRIPTS += test/functional/generate-cert.prereq

parallel_tests = $(BATS:.bats=.log)
$(parallel_tests) : test/functional/generate-cert.log

check_PROGRAMS = $(UNIT_TESTS)
LDADD = $(swupd_LDADD) $(swupd_OBJECTS:src/main.o=)

endif

if ENABLE_MANPAGE
MANPAGES = \
	docs/swupd.1 \
	docs/swupd-alias.7 \
	docs/check-update.service.4 \
	docs/check-update.timer.4 \
	docs/swupd-update.service.4 \
	docs/swupd-update.timer.4 \
	docs/update-triggers.target.4

dist_man_MANS = \
	$(MANPAGES)

clean-local:
	rm -rf $(MANPAGES)
endif

if COVERAGE
AM_CFLAGS += --coverage

coverage: coverage-clean
	mkdir -p coverage
	lcov --compat-libtool --directory . --capture --output-file coverage/report
	genhtml -o coverage/ coverage/report

coverage-clean:
	rm -rf coverage
endif

distclean-local:
	rm -rf aclocal.m4 ar-lib autom4te.cache config.guess config.h.in config.h.in~ config.sub configure depcomp install-sh ltmain.sh m4 Makefile.in missing compile

install-exec-hook:
	perl $(top_srcdir)/scripts/findstatic.pl */*.o | grep -v Checking ||:

compliant:
	@git diff --quiet --exit-code src; ret=$$?; \
	if [ $$ret -eq 1 ]; then \
		echo "Error: can only check code style when src/ are clean."; \
		echo "Stash or commit your changes and try again."; \
		exit $$ret; \
	elif [ $$ret -gt 1 ]; then \
		exit $$ret; \
	fi; \
	clang-format -i -style=file src/*.[ch] src/lib/*.[ch] ; ret=$$?; \
	if [ $$ret -ne 0 ]; then \
		exit $$ret; \
	fi; \
	git diff --quiet --exit-code src; ret=$$?; \
	if [ $$ret -eq 1 ]; then \
		echo "Code style issues found. Run 'git diff' to view issues."; \
	fi; \
	OUT=$$(git grep if\ \(.*\)$$ -- '*.c'); ret2=$$?; \
	if [ $$ret2 -eq 0 ]; then \
		echo "Missing brackets in single line if:"; \
		echo "$$OUT"; \
	elif [ $$ret -eq 0 ]; then \
		echo "No code style issues found."; \
		exit 0; \
	fi; \
	exit 1

# Ignore SC2207 because that's the standard way to fill COMPREPLY
# Ignore SC1008 because scripts are using an unsupported shebang
shellcheck:
	for i in $$(git diff --name-only origin/master HEAD | \grep -E "\.bats|\.bash"); do \
		echo checking "$$i"; \
		sed 's/^@.*/func() {/' "$$i" | \
		sed 's/^load.*/source test\/functional\/testlib.bash/' | \
		shellcheck -s bash -x -e SC1008 /dev/stdin; \
	done

shellcheck-all:
	shellcheck -e SC2207 swupd.bash
	find -name "*.bats" -exec sh -c "\
		echo checking {}; \
		sed 's/^@.*/func() {/' {} | \
		sed 's/^load.*/source test\/functional\/testlib.bash/' | \
		shellcheck -s bash -x -e SC1008 /dev/stdin \
		" \;

release:
	@git rev-parse v$(PACKAGE_VERSION) &> /dev/null; \
	if [ "$$?" -eq 0 ]; then \
		echo "Error: Release for $(PACKAGE_VERSION) already exists."; \
		echo "Bump version in configure.ac before releasing."; \
		exit 1; \
	fi
	@git tag -a -m "$(PACKAGE_NAME) release $(PACKAGE_VERSION)" v$(PACKAGE_VERSION)
	@printf "\nNew release $(PACKAGE_VERSION) tagged!\n\n"

man: $(dist_man_MANS)

.rst:
	mkdir -p "$$(dirname $@)"
	rst2man.py "$<" > "$@.tmp" && mv -f "$@.tmp" "$@"

if ENABLE_TESTS

install-check:
	test/installation/test.bats

unit-check:
	env TEST_SUITE_LOG=unit_tests.log TESTS=$(UNIT_TESTS) make -e check

endif
