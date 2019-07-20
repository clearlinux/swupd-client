SUFFIXES= .rst

EXTRA_DIST = COPYING scripts/findstatic.pl swupd.bash config

AM_CFLAGS = -fPIE -fPIC -g -D_FORTIFY_SOURCE=2 -fno-common -std=gnu99 \
	-O$(OPTIMIZE) \
	-Wall \
	-Wextra \
	-Wformat-security \
	-Wformat \
	-Wlogical-op \
	-Wunreachable-code \
	-Wswitch-default \
	-Wcast-align \
	-Wbad-function-cast \
	-Winline \
	-Wundef \
	-Wnested-externs \
	-Wredundant-decls \
	-fstack-protector-strong \
	-fno-strict-overflow \
	-fno-delete-null-pointer-checks \
	-fwrapv

AM_LDFLAGS = -z noexecstack -z relro -z now \
	-pie

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = swupd

AM_CPPFLAGS = $(AM_CFLAGS) $(libarchive_CFLAGS)

swupd_SOURCES = \
	src/alias.c \
	src/alias.h \
	src/autoupdate.c \
	src/binary_loader.c \
	src/bundle.c \
	src/check_update.c \
	src/clean.c \
	src/clr_bundle_add.c \
	src/clr_bundle_ls.c \
	src/clr_bundle_rm.c \
	src/config_loader.c \
	src/config_loader.h \
	src/curl.c \
	src/curl_async.c \
	src/delta.c \
	src/extra_files.c \
	src/filedesc.c \
	src/fullfile.c \
	src/globals.c \
	src/globals.h \
	src/hash.c \
	src/hashdump.c \
	src/helpers.c \
	src/heuristics.c \
	src/info.c \
	src/lib/archives.c \
	src/lib/archives.h \
	src/lib/config_parser.c \
	src/lib/config_parser.h \
	src/lib/formatter_json.c \
	src/lib/formatter_json.h \
	src/lib/hashmap.c \
	src/lib/hashmap.h \
	src/lib/list.c \
	src/lib/list.h \
	src/lib/log.c \
	src/lib/log.h \
	src/lib/macros.h \
	src/lib/progress.c \
	src/lib/progress.h \
	src/lib/strings.c \
	src/lib/strings.h \
	src/lib/sys.c \
	src/lib/sys.h \
	src/lib/thread_pool.c \
	src/lib/thread_pool.h \
	src/lock.c \
	src/main.c \
	src/manifest.c \
	src/manifest.h \
	src/manifest_parser.c \
	src/mirror.c \
	src/os_install.c \
	src/packs.c \
	src/repair.c \
	src/scripts.c \
	src/scripts.h \
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
	src/swupd_progress.c \
	src/swupd_progress.h \
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
	src/verifytime_main.c \
	src/lib/log.c \
	src/lib/log.h

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
	test/functional/bundleadd/add-also-add-flag.bats \
	test/functional/bundleadd/add-bad-hash-state.bats \
	test/functional/bundleadd/add-bad-hash.bats \
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
	test/functional/bundleadd/add-json.bats \
	test/functional/bundleadd/add-multiple.bats \
	test/functional/bundleadd/add-no-disk-space.bats \
	test/functional/bundleadd/add-no-signature.bats \
	test/functional/bundleadd/add-overwrite-files.bats \
	test/functional/bundleadd/add-skip-scripts.bats \
	test/functional/bundleadd/add-uses-fullfile.bats \
	test/functional/bundleadd/add-uses-zeropack.bats \
	test/functional/bundleadd/add-verify-fix-path.bats \
	test/functional/bundlelist/list-all.bats \
	test/functional/bundlelist/list-client-certificate.bats \
	test/functional/bundlelist/list-deps-flat.bats \
	test/functional/bundlelist/list-deps-invalid-bundle.bats \
	test/functional/bundlelist/list-deps-nested.bats \
	test/functional/bundlelist/list-experimental.bats \
	test/functional/bundlelist/list-has-dep-nested-not-installed.bats \
	test/functional/bundlelist/list-has-dep-nested-server.bats \
	test/functional/bundlelist/list-has-dep-nested.bats \
	test/functional/bundlelist/list-installed.bats \
	test/functional/bundlelist/list-json.bats \
	test/functional/bundlelist/list-no-deps.bats \
	test/functional/bundlelist/list-no-disk-space.bats \
	test/functional/bundlelist/list-none-has-deps.bats \
	test/functional/bundleremove/remove-basics.bats \
	test/functional/bundleremove/remove-boot-file.bats \
	test/functional/bundleremove/remove-client-certificate.bats \
	test/functional/bundleremove/remove-include-nested.bats \
	test/functional/bundleremove/remove-include.bats \
	test/functional/bundleremove/remove-json.bats \
	test/functional/bundleremove/remove-no-disk-space.bats \
	test/functional/bundleremove/remove-optional-bundles.bats \
	test/functional/bundleremove/remove-os-core.bats \
	test/functional/bundleremove/remove-parse-args.bats \
	test/functional/bundleremove/remove-with-dependency.bats \
	test/functional/checkupdate/chk-update-client-certificate.bats \
	test/functional/checkupdate/chk-update-json.bats \
	test/functional/checkupdate/chk-update-new-version.bats \
	test/functional/checkupdate/chk-update-no-server-content.bats \
	test/functional/checkupdate/chk-update-no-target-content.bats \
	test/functional/checkupdate/chk-update-slow-server.bats \
	test/functional/checkupdate/chk-update-version-match.bats \
	test/functional/diagnose/diagnose-also-add.bats \
	test/functional/diagnose/diagnose-basics.bats \
	test/functional/diagnose/diagnose-boot-file.bats \
	test/functional/diagnose/diagnose-client-certificate.bats \
	test/functional/diagnose/diagnose-directory-tree-deleted.bats \
	test/functional/diagnose/diagnose-flags.bats \
	test/functional/diagnose/diagnose-good.bats \
	test/functional/diagnose/diagnose-json.bats \
	test/functional/diagnose/diagnose-missing-file.bats \
	test/functional/diagnose/diagnose-picky-downgrade.bats \
	test/functional/diagnose/diagnose-picky-whitelist.bats \
	test/functional/diagnose/diagnose-picky.bats \
	test/functional/diagnose/diagnose-verify-flags.bats \
	test/functional/hashdump/hashdump-file-hash.bats \
	test/functional/mirror/mirror-createdir-negative.bats \
	test/functional/mirror/mirror-createdir.bats \
	test/functional/mirror/mirror-json.bats \
	test/functional/os-install/install-also-add.bats \
	test/functional/os-install/install-bad.bats \
	test/functional/os-install/install-basics.bats \
	test/functional/os-install/install-json.bats \
	test/functional/os-install/install-latest-missing.bats \
	test/functional/os-install/install-multiple.bats \
	test/functional/os-install/install-no-fullfile-fallback.bats \
	test/functional/os-install/install-no-packs.bats \
	test/functional/os-install/install-version.bats \
	test/functional/repair/repair-add-missing-include-old.bats \
	test/functional/repair/repair-add-missing-include.bats \
	test/functional/repair/repair-basics.bats \
	test/functional/repair/repair-boot-file-deleted.bats \
	test/functional/repair/repair-boot-file.bats \
	test/functional/repair/repair-boot-skip.bats \
	test/functional/repair/repair-directory-tree-deleted.bats \
	test/functional/repair/repair-empty-dir-deleted.bats \
	test/functional/repair/repair-flags.bats \
	test/functional/repair/repair-format-mismatch-overdrive.bats \
	test/functional/repair/repair-format-mismatch.bats \
	test/functional/repair/repair-ghosted.bats \
	test/functional/repair/repair-ignore-also-add.bats \
	test/functional/repair/repair-json.bats \
	test/functional/repair/repair-missing-file.bats \
	test/functional/repair/repair-no-disk-space.bats \
	test/functional/repair/repair-picky.bats \
	test/functional/repair/repair-picky-downgrade.bats \
	test/functional/repair/repair-picky-ghosted-missing.bats \
	test/functional/repair/repair-picky-ghosted.bats \
	test/functional/repair/repair-skip-scripts.bats \
	test/functional/repair/repair-unsafe-to-delete.bats \
	test/functional/repair/repair-version-mismatch-overdrive.bats \
	test/functional/repair/repair-version-mismatch.bats \
	test/functional/search/search-client-certificate.bats \
	test/functional/search/search-content-check-negative.bats \
	test/functional/search/search-content-check-positive.bats \
	test/functional/search/search-experimental.bats \
	test/functional/search/search-json.bats \
	test/functional/search/search-no-disk-space.bats \
	test/functional/search/search-sort.bats \
	test/functional/signature/cert-chain.bats \
	test/functional/signature/corrupted-certificate.bats \
	test/functional/signature/corrupted-signature.bats \
	test/functional/signature/invalid-certificate.bats \
	test/functional/signature/key-rotation.bats \
	test/functional/signature/no-signature.bats \
	test/functional/signature/permission-incorrect.bats \
	test/functional/update/update-boot-file.bats \
	test/functional/update/update-boot-skip.bats \
	test/functional/update/update-bundle-removed.bats \
	test/functional/update/update-client-certificate.bats \
	test/functional/update/update-download.bats \
	test/functional/update/update-fail-to-get-mom.bats \
	test/functional/update/update-ignore-also-add.bats \
	test/functional/update/update-include-old-bundle-with-tracked-file.bats \
	test/functional/update/update-include-old-bundle.bats \
	test/functional/update/update-include.bats \
	test/functional/update/update-json.bats \
	test/functional/update/update-manifest-delta.bats \
	test/functional/update/update-minversion.bats \
	test/functional/update/update-missing-os-core.bats \
	test/functional/update/update-newest-added.bats \
	test/functional/update/update-newest-deleted.bats \
	test/functional/update/update-newest-ghosted.bats \
	test/functional/update/update-no-disk-space.bats \
	test/functional/update/update-non-responsive.bats \
	test/functional/update/update-older-server-version.bats \
	test/functional/update/update-re-update-bad-os-release.bats \
	test/functional/update/update-re-update-required.bats \
	test/functional/update/update-rename-ghosted.bats \
	test/functional/update/update-rename.bats \
	test/functional/update/update-show-time.bats \
	test/functional/update/update-skip-scripts.bats \
	test/functional/update/update-slow-server.bats \
	test/functional/update/update-statedir-bad-hash.bats \
	test/functional/update/update-status-no-server-content.bats \
	test/functional/update/update-status-no-target-content.bats \
	test/functional/update/update-status.bats \
	test/functional/update/update-use-full-file.bats \
	test/functional/update/update-use-pack.bats \
	test/functional/update/update-verify-fix-path-hash-mismatch.bats \
	test/functional/update/update-verify-fix-path-missing-dir.bats \
	test/functional/update/update-verify-fullfile-hash.bats \
	test/functional/update/update-with-mirror.bats \
	test/functional/update/update-with-old-mirror.bats \
	test/functional/update/update-with-slightly-old-mirror.bats \
	test/functional/usability/usa-completion-basic.bats \
	test/functional/usability/usa-config-file.bats \
	test/functional/usability/usa-download-retries.bats \
	test/functional/usability/usa-external-modules.bats \
	test/functional/verify-legacy/verify-bundle.bats


UNIT_TESTS = \
	test/unit/test_signature.test \
	test/unit/test_strings.test \
	test/unit/test_manifest.test

EXTRA_DIST += test/unit/test_helper.h test/unit/data test/functional

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

# Ignore SC1008 because scripts are using an unsupported shebang
shellcheck:
	test/code_analysis/shellcheck.bats

shellcheck-all:
	test/code_analysis/shellcheck-all.bats

check-test-ids:
	test/functional/check_ids.bash

check-flags:
	scripts/flag_validator.bash

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
	env TEST_SUITE_LOG=unit_tests.log TESTS="$(UNIT_TESTS)" make -e check

docs-coverage:
	doxygen docs/Doxyfile || die; \
	python -m coverxygen --xml-dir xml/ --src-dir ./ --output doc-coverage.info --exclude ".*/src/.*" --include ".*/src/lib/.*" --format json || die; \
	if [ "$$(cat doc-coverage.info | grep documented.*false | wc -l)" -gt "0" ]; then \
		echo "Public functions or data added on headers but not documented on src/lib "; \
		exit 1; \
	fi

endif
