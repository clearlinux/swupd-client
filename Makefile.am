SUFFIXES= .rst

EXTRA_DIST = COPYING scripts/findstatic.pl swupd.bash

AM_CFLAGS = -fPIC -Iinclude/ -O2 -g -Wall -W -Wformat-security -D_FORTIFY_SOURCE=2 -fno-common -std=gnu99
ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = swupd

swupd_SOURCES = \
	src/autoupdate.c \
	src/check_update.c \
	src/clean.c \
	src/clr_bundle_add.c \
	src/clr_bundle_ls.c \
	src/clr_bundle_rm.c \
	src/extra_files.c \
	src/hashdump.c \
	src/info.c \
	src/main.c \
	src/search.c \
	src/swupd.c \
	src/verify.c

SWUPD_COMMON_SOURCES = \
	src/archives.c \
	src/curl.c \
	src/delta.c \
	src/download.c \
	src/filedesc.c \
	src/hash.c \
	src/helpers.c \
	src/heuristics.c \
	src/list.c \
	src/lock.c \
	src/manifest.c \
	src/packs.c \
	src/signature.c \
	src/staging.c \
	src/stats.c \
	src/subscriptions.c \
	src/update.c \
	src/version.c \
	src/xattrs.c \
	src/globals.c \
	src/scripts.c \
	src/bundle.c \
	src/telemetry.c

lib_LTLIBRARIES = libswupd.la
libswupd_la_SOURCES = $(SWUPD_COMMON_SOURCES)
libswupd_la_LIBADD = $(SWUPD_COMPRESSION_LIBS) $(openssl_LIBS) $(curl_LIBS) $(bsdiff_LIBS) $(libarchive_LIBS)
# Library version changes according to the libtool convention:
# http://www.gnu.org/software/libtool/manual/libtool.html#Updating-version-info
LIBSWUPD_CURRENT=2
LIBSWUPD_REVISION=0
LIBSWUPD_AGE=0
libswupd_la_LDFLAGS = \
	-version-info $(LIBSWUPD_CURRENT):$(LIBSWUPD_REVISION):$(LIBSWUPD_AGE)

AM_CPPFLAGS = $(AM_CFLAGS) $(libarchive_CFLAGS) -I$(top_srcdir)/include
SWUPD_COMPRESSION_LIBS = $(lzma_LIBS) $(zlib_LIBS) $(bzip2_LIBS)
SWUPD_CORE_LIBS = libswupd.la

swupd_LDADD = $(SWUPD_CORE_LIBS) $(pthread_LIBS)

verifytime_SOURCES = src/verifytime.c
bin_PROGRAMS += verifytime

dist_bin_SCRIPTS = swupd-add-pkg

noinst_HEADERS = $(top_srcdir)/include/*

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

distclean-local:
	rm -rf aclocal.m4 ar-lib autom4te.cache config.guess config.h.in config.h.in~ config.sub configure depcomp install-sh ltmain.sh m4 Makefile.in missing compile

install-exec-hook:
	perl $(top_srcdir)/scripts/findstatic.pl */*.o | grep -v Checking ||:


TEST_EXTENSIONS = .bats .prereq

if ENABLE_TESTS
tap_driver = env AM_TAP_AWK='$(AWK)' $(SHELL) \
	$(top_srcdir)/tap-driver.sh

LOG_DRIVER = $(tap_driver)
BATS_LOG_DRIVER = $(tap_driver)

TESTS = $(dist_check_SCRIPTS)

BATS = \
	test/functional/bundleadd/add-directory/test.bats \
	test/functional/bundleadd/add-existing/test.bats \
	test/functional/bundleadd/add-multiple/test.bats \
	test/functional/bundleadd/add-rc/test.bats \
	test/functional/bundleadd/bad-hash/test.bats \
	test/functional/bundleadd/bad-hash-state/test.bats \
	test/functional/bundleadd/bad-manifest/test.bats \
	test/functional/bundleadd/boot-file/test.bats \
	test/functional/bundleadd/boot-skip/test.bats \
	test/functional/bundleadd/include/test.bats \
	test/functional/bundleadd/fix-missing-file/test.bats \
	test/functional/bundleadd/skip-scripts/test.bats \
	test/functional/bundleadd/verify-fix-path/test.bats \
	test/functional/bundlelist/all/test.bats \
	test/functional/bundlelist/deps-flat/test.bats \
	test/functional/bundlelist/deps-invalid-bundle/test.bats \
	test/functional/bundlelist/deps-nested/test.bats \
	test/functional/bundlelist/has-dep-nested/test.bats \
	test/functional/bundlelist/has-dep-nested-not-installed/test.bats \
	test/functional/bundlelist/has-dep-nested-server/test.bats \
	test/functional/bundlelist/no-deps/test.bats \
	test/functional/bundlelist/none-has-deps/test.bats \
	test/functional/bundleremove/boot-file/test.bats \
	test/functional/bundleremove/include/test.bats \
	test/functional/bundleremove/include-nested/test.bats \
	test/functional/bundleremove/parse-args/test.bats \
	test/functional/bundleremove/remove-file/test.bats \
	test/functional/bundleremove/remove-multiple/test.bats \
	test/functional/checkupdate/new-version/test.bats \
	test/functional/checkupdate/no-server-content/test.bats \
	test/functional/checkupdate/no-target-content/test.bats \
	test/functional/checkupdate/slow-server/test.bats \
	test/functional/checkupdate/version-match/test.bats \
	test/functional/completion/basic/test.bats \
	test/functional/hashdump/file-hash/test.bats \
	test/functional/hashdump/file-hash-no-path-prefix/test.bats \
	test/functional/search/content-check-negfull-path/test.bats \
	test/functional/search/content-check-neglibtest/test.bats \
	test/functional/search/content-check-posbin/test.bats \
	test/functional/search/content-check-posebin/test.bats \
	test/functional/search/content-check-posfull-path/test.bats \
	test/functional/search/content-check-poslib32/test.bats \
	test/functional/search/content-check-poslib64/test.bats \
	test/functional/update/apply-full-file-delta/test.bats \
	test/functional/update/boot-file/test.bats \
	test/functional/update/boot-skip/test.bats \
	test/functional/update/download/test.bats \
	test/functional/update/include/test.bats \
	test/functional/update/include-old-bundle/test.bats \
	test/functional/update/include-old-bundle-with-tracked-file/test.bats \
	test/functional/update/missing-os-core/test.bats \
	test/functional/update/newest-deleted/test.bats \
	test/functional/update/newest-ghosted/test.bats \
	test/functional/update/rename/test.bats \
	test/functional/update/rename-ghosted/test.bats \
	test/functional/update/skip-scripts/test.bats \
	test/functional/update/skip-verified-fullfiles/test.bats \
	test/functional/update/status/test.bats \
	test/functional/update/status-no-server-content/test.bats \
	test/functional/update/status-no-target-content/test.bats \
	test/functional/update/status-version-double-quote/test.bats \
	test/functional/update/status-version-single-quote/test.bats \
	test/functional/update/use-full-file/test.bats \
	test/functional/update/use-pack/test.bats \
	test/functional/update/verify-fix-path-hash-mismatch/test.bats \
	test/functional/update/verify-fix-path-missing-dir/test.bats \
	test/functional/update/verify-fullfile-hash/test.bats \
	test/functional/update/re-update-required/test.bats \
	test/functional/update/re-update-bad-os-release/test.bats \
	test/functional/update/statedir-bad-hash/test.bats \
	test/functional/verify/add-missing-directory/test.bats \
	test/functional/verify/add-missing-include/test.bats \
	test/functional/verify/add-missing-include-old/test.bats \
	test/functional/verify/boot-file-deleted/test.bats \
	test/functional/verify/boot-file-mismatch/test.bats \
	test/functional/verify/boot-file-mismatch-fix/test.bats \
	test/functional/verify/boot-skip/test.bats \
	test/functional/verify/check-missing-directory/test.bats \
	test/functional/verify/directory-tree-deleted/test.bats \
	test/functional/verify/empty-dir-deleted/test.bats \
	test/functional/verify/fix-version-mismatch/test.bats \
	test/functional/verify/fix-version-mismatch-override/test.bats \
	test/functional/verify/format-mismatch/test.bats \
	test/functional/verify/format-mismatch-override/test.bats \
	test/functional/verify/ghosted/test.bats \
	test/functional/verify/install-directory/test.bats \
	test/functional/verify/install-latest-directory/test.bats \
	test/functional/verify/latest-missing/test.bats \
	test/functional/verify/picky-ghosted/test.bats \
	test/functional/verify/picky-ghosted-missing/test.bats \
	test/functional/verify/skip-scripts/test.bats

dist_check_SCRIPTS = $(BATS)
# Must be run before all other tests
dist_check_SCRIPTS += test/functional/generate-cert.prereq

parallel_tests = $(BATS:.bats=.log)
$(parallel_tests) : test/functional/generate-cert.log
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

compliant:
	@git diff --quiet --exit-code include src; ret=$$?; \
	if [ $$ret -eq 1 ]; then \
		echo "Error: can only check code style when include/ and src/ are clean."; \
		echo "Stash or commit your changes and try again."; \
		exit $$ret; \
	elif [ $$ret -gt 1 ]; then \
		exit $$ret; \
	fi; \
	clang-format -i -style=file include/*.h src/*.c; ret=$$?; \
	if [ $$ret -ne 0 ]; then \
		exit $$ret; \
	fi; \
	git diff --quiet --exit-code include src; ret=$$?; \
	if [ $$ret -eq 1 ]; then \
		echo "Code style issues found. Run 'git diff' to view issues."; \
	elif [ $$ret -eq 0 ]; then \
		echo "No code style issues found."; \
	fi; \
	exit $$ret

release:
	@git rev-parse v$(PACKAGE_VERSION) &> /dev/null; \
	if [ "$$?" -eq 0 ]; then \
		echo "Error: Release for $(PACKAGE_VERSION) already exists."; \
		echo "Bump version in configure.ac before releasing."; \
		exit 1; \
	fi
	@git tag -a -m "$(PACKAGE_NAME) release $(PACKAGE_VERSION)" v$(PACKAGE_VERSION)
	@printf "\nNew release $(PACKAGE_VERSION) tagged!\n\n"


MANPAGES = \
	docs/swupd.1 \
	docs/check-update.service.4 \
	docs/check-update.timer.4 \
	docs/swupd-update.service.4 \
	docs/swupd-update.timer.4 \
	docs/update-triggers.target.4 \
	docs/swupd-add-pkg.4

dist_man_MANS = \
	$(MANPAGES)

clean-local:
	rm -f $(dist_man_MANS)

man: $(dist_man_MANS)

.rst:
	mkdir -p "$$(dirname $@)"
	rst2man.py "$<" > "$@.tmp" && mv -f "$@.tmp" "$@"

test/functional/completion/basic/test.bats: swupd.bash

swupd.bash: $(top_srcdir)/scripts/swupd-completion.sh $(top_srcdir)/swupd
	bash $<
