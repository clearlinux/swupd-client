SUFFIXES= .rst

EXTRA_DIST = COPYING scripts/findstatic.pl swupd.bash swupd.zsh config

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
	src/3rd_party_add.c \
	src/3rd_party_bundle_add.c \
	src/3rd_party_bundle_list.c \
	src/3rd_party.c \
	src/3rd_party_list.c \
	src/3rd_party_remove.c \
	src/3rd_party_repos.c \
	src/3rd_party_repos.h \
	src/alias.c \
	src/alias.h \
	src/autoupdate.c \
	src/binary_loader.c \
	src/bundle_add.c \
	src/bundle.c \
	src/bundle.h \
	src/bundle_info.c \
	src/bundle_list.c \
	src/bundle_remove.c \
	src/check_update.c \
	src/clean.c \
	src/config_loader.c \
	src/config_loader.h \
	src/curl_async.c \
	src/curl.c \
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
	src/lib/config_file.c \
	src/lib/config_file.h \
	src/lib/formatter_json.c \
	src/lib/formatter_json.h \
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
	src/lib/timelist.c \
	src/lib/timelist.h \
	src/lock.c \
	src/main.c \
	src/manifest.c \
	src/manifest.h \
	src/manifest_parser.c \
	src/mirror.c \
	src/os_install.c \
	src/packs.c \
	src/progress.c \
	src/progress.h \
	src/repair.c \
	src/scripts.c \
	src/scripts.h \
	src/search_file.c \
	src/signature.c \
	src/signature.h \
	src/staging.c \
	src/stats.c \
	src/subscriptions.c \
	src/swupd_build_opts.h \
	src/swupd_build_variant.h \
	src/swupd_curl.h \
	src/swupd_curl_internal.h \
	src/swupd_exit_codes.h \
	src/swupd.h \
	src/swupd_internal.h \
	src/swupd_progress.c \
	src/swupd_progress.h \
	src/telemetry.c \
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

TESTS = \
	test/unit/test_list.test \
	test/unit/test_manifest.test \
	test/unit/test_signature.test \
	test/unit/test_strings.test

EXTRA_DIST += test/unit/test_helper.h test/unit/data

check_PROGRAMS = $(TESTS)
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
	rm -rf aclocal.m4 ar-lib autom4te.cache config.guess config.h.in config.h.in~ config.sub configure depcomp install-sh ltmain.sh m4 Makefile.in missing compile Swupd_Root.pem private.pem $(MANPAGES)

install-exec-hook:
	perl $(top_srcdir)/scripts/findstatic.pl */*.o | grep -v Checking ||:

compliant:
	test/code_analysis/compliant.bats
	test/functional/check_ids.bash
	scripts/flag_validator.bash

# Ignore SC1008 because scripts are using an unsupported shebang
shellcheck:
	test/code_analysis/shellcheck.bats

shellcheck-all:
	test/code_analysis/shellcheck-all.bats

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

functional-check:
	test/functional/generate-cert.prereq
	env TESTS="$(shell find test/functional  -name *.bats)" make -e check

docs-coverage:
	doxygen docs/Doxyfile || die; \
	python -m coverxygen --xml-dir xml/ --src-dir ./ --output doc-coverage.info --exclude ".*/src/.*" --include ".*/src/lib/.*" --format json || die; \
	if [ "$$(cat doc-coverage.info | grep documented.*false | wc -l)" -gt "0" ]; then \
		echo "Public functions or data added on headers but not documented on src/lib "; \
		exit 1; \
	fi

endif
