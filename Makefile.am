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
	test/functional/bundlelist/list-all.bats \
	test/functional/bundlelist/list-deps-flat.bats \
	test/functional/bundlelist/list-deps-invalid-bundle.bats \
	test/functional/bundlelist/list-deps-nested.bats \
	test/functional/bundlelist/list-has-dep-nested.bats \
	test/functional/bundlelist/list-has-dep-nested-not-installed.bats \
	test/functional/bundlelist/list-has-dep-nested-server.bats \
	test/functional/bundlelist/list-no-deps.bats \
	test/functional/bundlelist/list-none-has-deps.bats \
	test/functional/bundlelist/list-client-certificate.bats \
	test/functional/bundleremove/remove-boot-file.bats \
	test/functional/bundleremove/remove-include.bats \
	test/functional/bundleremove/remove-include-nested.bats \
	test/functional/bundleremove/remove-os-core.bats \
	test/functional/bundleremove/remove-parse-args.bats \
	test/functional/bundleremove/remove-file.bats \
	test/functional/bundleremove/remove-multiple.bats \
	test/functional/bundleremove/remove-rc.bats \
	test/functional/bundleremove/remove-client-certificate.bats \
	test/functional/checkupdate/chk-update-new-version.bats \
	test/functional/checkupdate/chk-update-no-server-content.bats \
	test/functional/checkupdate/chk-update-no-target-content.bats \
	test/functional/checkupdate/chk-update-slow-server.bats \
	test/functional/checkupdate/chk-update-version-match.bats \
	test/functional/checkupdate/chk-update-client-certificate.bats \
	test/functional/completion/completion-basic.bats \
	test/functional/hashdump/hashdump-file-hash.bats \
	test/functional/mirror/mirror-createdir.bats \
	test/functional/mirror/mirror-createdir-negative.bats \
	test/functional/search/search-content-check-negative.bats \
	test/functional/search/search-content-check-positive.bats \
	test/functional/search/search-client-certificate.bats \
	test/functional/update/update-apply-full-file-delta.bats \
	test/functional/update/update-boot-file.bats \
	test/functional/update/update-boot-skip.bats \
	test/functional/update/update-client-certificate.bats \
	test/functional/update/update-delta-without-rename-flag.bats \
	test/functional/update/update-download.bats \
	test/functional/update/update-include.bats \
	test/functional/update/update-include-old-bundle.bats \
	test/functional/update/update-include-old-bundle-with-tracked-file.bats \
	test/functional/update/update-missing-os-core.bats \
	test/functional/update/update-newest-deleted.bats \
	test/functional/update/update-newest-ghosted.bats \
	test/functional/update/update-rename.bats \
	test/functional/update/update-rename-ghosted.bats \
	test/functional/update/update-re-update-bad-os-release.bats \
	test/functional/update/update-re-update-required.bats \
	test/functional/update/update-skip-scripts.bats \
	test/functional/update/update-skip-verified-fullfiles.bats \
	test/functional/update/update-slow-server.bats \
	test/functional/update/update-statedir-bad-hash.bats \
	test/functional/update/update-status.bats \
	test/functional/update/update-status-no-server-content.bats \
	test/functional/update/update-status-no-target-content.bats \
	test/functional/update/update-status-version-double-quote.bats \
	test/functional/update/update-status-version-single-quote.bats \
	test/functional/update/update-use-full-file.bats \
	test/functional/update/update-use-pack.bats \
	test/functional/update/update-verify-fix-path-hash-mismatch.bats \
	test/functional/update/update-verify-fix-path-missing-dir.bats \
	test/functional/update/update-verify-fullfile-hash.bats \
	test/functional/verify/verify-add-missing-directory.bats \
	test/functional/verify/verify-add-missing-include.bats \
	test/functional/verify/verify-add-missing-include-old.bats \
	test/functional/verify/verify-boot-file-deleted.bats \
	test/functional/verify/verify-boot-file-mismatch.bats \
	test/functional/verify/verify-boot-file-mismatch-fix.bats \
	test/functional/verify/verify-boot-skip.bats \
	test/functional/verify/verify-check-missing-directory.bats \
	test/functional/verify/verify-client-certificate.bats
	test/functional/verify/verify-directory-tree-deleted.bats \
	test/functional/verify/verify-empty-dir-deleted.bats \
	test/functional/verify/verify-fix-version-mismatch.bats \
	test/functional/verify/verify-fix-version-mismatch-override.bats \
	test/functional/verify/verify-format-mismatch.bats \
	test/functional/verify/verify-format-mismatch-override.bats \
	test/functional/verify/verify-ghosted.bats \
	test/functional/verify/verify-install-directory.bats \
	test/functional/verify/verify-install-latest-directory.bats \
	test/functional/verify/verify-latest-missing.bats \
	test/functional/verify/verify-picky-ghosted.bats \
	test/functional/verify/verify-picky-ghosted-missing.bats \
	test/functional/verify/verify-skip-scripts.bats

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

test/functional/completion/completion-basic.bats: swupd.bash
