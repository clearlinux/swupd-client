SUFFIXES= .rst

EXTRA_DIST = COPYING scripts/findstatic.pl swupd.bash

AM_CFLAGS = -fPIC -O2 -g -Wall -W -Wformat-security -D_FORTIFY_SOURCE=2 -fno-common -std=gnu99
ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = swupd

AM_CPPFLAGS = $(AM_CFLAGS) $(libarchive_CFLAGS)

swupd_SOURCES = \
	src/archives.c \
	src/autoupdate.c \
	src/bundle.c \
	src/check_update.c \
	src/clean.c \
	src/clr_bundle_add.c \
	src/clr_bundle_ls.c \
	src/clr_bundle_rm.c \
	src/curl-internal.h \
	src/curl.c \
	src/datatypes.h \
	src/delta.c \
	src/download.c \
	src/extra_files.c \
	src/filedesc.c \
	src/fs.c \
	src/fullfile.c \
	src/globals.c \
	src/hash.c \
	src/hashdump.c \
	src/hashmap.c \
	src/hashmap.h \
	src/helpers.c \
	src/heuristics.c \
	src/info.c \
	src/list.c \
	src/list.h \
	src/lock.c \
	src/macros.h \
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
	src/swupd-build-opts.h \
	src/swupd-build-variant.h \
	src/swupd-error.h \
	src/swupd-internal.h \
	src/swupd.h \
	src/telemetry.c \
	src/update.c \
	src/verify.c \
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
	$(libarchive_LIBS)

verifytime_SOURCES = src/verifytime.c
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
	test/documentation/manpages/test.bats \
	test/functional/bundleadd/add-directory.bats \
	test/functional/bundleadd/add-existing.bats \
	test/functional/bundleadd/add-multiple.bats \
	test/functional/bundleadd/add-rc.bats \
	test/functional/bundleadd/add-bad-hash.bats \
	test/functional/bundleadd/add-bad-hash-state.bats \
	test/functional/bundleadd/add-bad-manifest.bats \
	test/functional/bundleadd/add-boot-file.bats \
	test/functional/bundleadd/add-boot-skip.bats \
	test/functional/bundleadd/add-include.bats \
	test/functional/bundleadd/add-skip-scripts.bats \
	test/functional/bundleadd/add-verify-fix-path.bats \
	test/functional/bundleadd/add-client-certificate.bats \
	test/functional/bundlelist/all/test.bats \
	test/functional/bundlelist_v2/list-all.bats \
	test/functional/bundlelist/deps-flat/test.bats \
	test/functional/bundlelist_v2/list-deps-flat.bats \
	test/functional/bundlelist/deps-invalid-bundle/test.bats \
	test/functional/bundlelist_v2/list-deps-invalid-bundle.bats \
	test/functional/bundlelist/deps-nested/test.bats \
	test/functional/bundlelist_v2/list-deps-nested.bats \
	test/functional/bundlelist/has-dep-nested/test.bats \
	test/functional/bundlelist_v2/list-has-dep-nested.bats \
	test/functional/bundlelist/has-dep-nested-not-installed/test.bats \
	test/functional/bundlelist_v2/list-has-dep-nested-not-installed.bats \
	test/functional/bundlelist/has-dep-nested-server/test.bats \
	test/functional/bundlelist_v2/list-has-dep-nested-server.bats \
	test/functional/bundlelist/no-deps/test.bats \
	test/functional/bundlelist_v2/list-no-deps.bats \
	test/functional/bundlelist/none-has-deps/test.bats \
	test/functional/bundlelist_v2/list-none-has-deps.bats \
	test/functional/bundlelist_v2/list-client-certificate.bats \
	test/functional/bundleremove/boot-file/test.bats \
	test/functional/bundleremove_v2/remove-boot-file.bats \
	test/functional/bundleremove/include/test.bats \
	test/functional/bundleremove_v2/remove-include.bats \
	test/functional/bundleremove/include-nested/test.bats \
	test/functional/bundleremove_v2/remove-include-nested.bats \
	test/functional/bundleremove_v2/remove-os-core.bats \
	test/functional/bundleremove/parse-args/test.bats \
	test/functional/bundleremove_v2/remove-parse-args.bats \
	test/functional/bundleremove/remove-file/test.bats \
	test/functional/bundleremove_v2/remove-file.bats \
	test/functional/bundleremove/remove-multiple/test.bats \
	test/functional/bundleremove_v2/remove-multiple.bats \
	test/functional/bundleremove_v2/remove-rc.bats \
	test/functional/bundleremove_v2/remove-client-certificate.bats \
	test/functional/checkupdate/new-version/test.bats \
	test/functional/checkupdate_v2/chk-update-new-version.bats \
	test/functional/checkupdate/no-server-content/test.bats \
	test/functional/checkupdate_v2/chk-update-no-server-content.bats \
	test/functional/checkupdate/no-target-content/test.bats \
	test/functional/checkupdate_v2/chk-update-no-target-content.bats \
	test/functional/checkupdate/slow-server/test.bats \
	test/functional/checkupdate_v2/chk-update-slow-server.bats \
	test/functional/checkupdate/version-match/test.bats \
	test/functional/checkupdate_v2/chk-update-version-match.bats \
	test/functional/checkupdate_v2/chk-update-client-certificate.bats \
	test/functional/completion/basic/test.bats \
	test/functional/completion_v2/completion-basic.bats \
	test/functional/hashdump/file-hash/test.bats \
	test/functional/hashdump/file-hash-no-path-prefix/test.bats \
	test/functional/hashdump_v2/hashdump-file-hash.bats \
	test/functional/mirror/createdir/test.bats \
	test/functional/mirror_v2/mirror-createdir.bats \
	test/functional/mirror/createdir-negative/test.bats \
	test/functional/mirror_v2/mirror-createdir-negative.bats \
	test/functional/search/content-check-negfull-path/test.bats \
	test/functional/search/content-check-neglibtest/test.bats \
	test/functional/search_v2/search-content-check-negative.bats \
	test/functional/search/content-check-posbin/test.bats \
	test/functional/search/content-check-posebin/test.bats \
	test/functional/search/content-check-posfull-path/test.bats \
	test/functional/search/content-check-poslib32/test.bats \
	test/functional/search/content-check-poslib64/test.bats \
	test/functional/search_v2/search-content-check-positive.bats \
	test/functional/search_v2/search-client-certificate.bats \
	test/functional/update/apply-full-file-delta/test.bats \
	test/functional/update/boot-file/test.bats \
	test/functional/update/boot-skip/test.bats \
	test/functional/update/delta-without-rename-flag/test.bats \
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
	test/functional/update/slow-server/test.bats \
	test/functional/update/statedir-bad-hash/test.bats \
	test/functional/update_v2/update-client-certificate.bats \
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
	test/functional/verify/skip-scripts/test.bats \
	test/functional/verify_v2/verify-client-certificate.bats

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
	@git diff --quiet --exit-code src; ret=$$?; \
	if [ $$ret -eq 1 ]; then \
		echo "Error: can only check code style when src/ are clean."; \
		echo "Stash or commit your changes and try again."; \
		exit $$ret; \
	elif [ $$ret -gt 1 ]; then \
		exit $$ret; \
	fi; \
	clang-format -i -style=file src/*.h src/*.c; ret=$$?; \
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
	docs/update-triggers.target.4

dist_man_MANS = \
	$(MANPAGES)

man: $(dist_man_MANS)

.rst:
	mkdir -p "$$(dirname $@)"
	rst2man.py "$<" > "$@.tmp" && mv -f "$@.tmp" "$@"

test/functional/completion/basic/test.bats: swupd.bash
