SUFFIXES= .rst

EXTRA_DIST = COPYING scripts/findstatic.pl swupd.bash swupd.zsh config

AM_CFLAGS = -fPIE -fPIC -g -fno-common -std=gnu99 \
	-O$(OPTIMIZE) \
	-Wall \
	-Wextra \
	-Wformat-security \
	-Wformat \
	-Wlogical-op \
	-Wunreachable-code \
	-Wswitch-default \
	-Wcast-align \
	-Wshadow \
	-Wbad-function-cast \
	-Winline \
	-Wundef \
	-Wnested-externs \
	-Wredundant-decls \
	-fstack-protector-strong \
	-fno-strict-overflow \
	-fno-delete-null-pointer-checks \
	-fwrapv \
	-include "config.h" \
	-I$(top_srcdir)/src/

if ENABLE_FORTIFY
AM_CFLAGS += -D_FORTIFY_SOURCE=2
endif

AM_LDFLAGS = -z noexecstack -z relro -z now \
	-pie

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = swupd

AM_CPPFLAGS = $(AM_CFLAGS) $(libarchive_CFLAGS)

swupd_SOURCES = \
	src/3rd_party/3rd_party_add.c \
	src/3rd_party/3rd_party_bundle_add.c \
	src/3rd_party/3rd_party_bundle_info.c \
	src/3rd_party/3rd_party_bundle_list.c \
	src/3rd_party/3rd_party_bundle_remove.c \
	src/3rd_party/3rd_party.c \
	src/3rd_party/3rd_party_check_update.c \
	src/3rd_party/3rd_party_clean.c \
	src/3rd_party/3rd_party_diagnose.c \
	src/3rd_party/3rd_party_info.c \
	src/3rd_party/3rd_party_list.c \
	src/3rd_party/3rd_party_remove.c \
	src/3rd_party/3rd_party_repair.c \
	src/3rd_party/3rd_party_repos.c \
	src/3rd_party/3rd_party_repos.h \
	src/3rd_party/3rd_party_update.c \
	src/cmds/autoupdate.c \
	src/cmds/bundle_add.c \
	src/cmds/bundle_info.c \
	src/cmds/bundle_list.c \
	src/cmds/bundle_remove.c \
	src/cmds/check_update.c \
	src/cmds/clean.c \
	src/cmds/info.c \
	src/cmds/os_install.c \
	src/cmds/repair.c \
	src/cmds/swupd_cmds.h \
	src/cmds/update.c \
	src/cmds/verify.c \
	src/lib/archives.c \
	src/lib/archives.h \
	src/lib/comp_functions.c \
	src/lib/comp_functions.h \
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
	src/main.c \
	src/swupd_build_opts.h \
	src/swupd_build_variant.h \
	src/swupd_exit_codes.h \
	src/swupd.h \
	src/swupd_lib/alias.c \
	src/swupd_lib/alias.h \
	src/swupd_lib/binary_loader.c \
	src/swupd_lib/bundle.c \
	src/swupd_lib/bundle.h \
	src/swupd_lib/config_loader.c \
	src/swupd_lib/config_loader.h \
	src/swupd_lib/curl_async.c \
	src/swupd_lib/curl.c \
	src/swupd_lib/delta.c \
	src/swupd_lib/extra_files.c \
	src/swupd_lib/filedesc.c \
	src/swupd_lib/fullfile.c \
	src/swupd_lib/globals.c \
	src/swupd_lib/globals.h \
	src/swupd_lib/hash.c \
	src/swupd_lib/hashdump.c \
	src/swupd_lib/helpers.c \
	src/swupd_lib/heuristics.c \
	src/swupd_lib/heuristics.h \
	src/swupd_lib/lock.c \
	src/swupd_lib/manifest.c \
	src/swupd_lib/manifest.h \
	src/swupd_lib/manifest_parser.c \
	src/swupd_lib/mirror.c \
	src/swupd_lib/packs.c \
	src/swupd_lib/progress.c \
	src/swupd_lib/progress.h \
	src/swupd_lib/scripts.c \
	src/swupd_lib/scripts.h \
	src/swupd_lib/search_file.c \
	src/swupd_lib/signature.c \
	src/swupd_lib/signature.h \
	src/swupd_lib/stats.c \
	src/swupd_lib/subscriptions.c \
	src/swupd_lib/swupd_comp_functions.c \
	src/swupd_lib/swupd_comp_functions.h \
	src/swupd_lib/swupd_curl.h \
	src/swupd_lib/swupd_curl_internal.h \
	src/swupd_lib/swupd_progress.c \
	src/swupd_lib/swupd_progress.h \
	src/swupd_lib/target_root.c \
	src/swupd_lib/target_root.h \
	src/swupd_lib/telemetry.c \
	src/swupd_lib/telemetry.h \
	src/swupd_lib/version.c \
	src/swupd_lib/xattrs.c \
	src/swupd_lib/xattrs.h \
	src/verifytime/verifytime.c

swupd_LDADD = \
	$(lzma_LIBS) \
	$(zlib_LIBS) \
	$(bzip2_LIBS) \
	$(openssl_LIBS) \
	$(curl_LIBS) \
	$(bsdiff_LIBS) \
	$(libarchive_LIBS) \
	$(pthread_LIBS)

verifytime_SOURCES = \
	src/verifytime/verifytime.h \
	src/verifytime/verifytime.c \
	src/verifytime/verifytime_main.c

verifytime_LDADD =

bin_PROGRAMS += verifytime

EXTRA_DIST += \
	data/swupd-update.service \
	data/swupd-update.timer \
	data/verifytime.service

DISTCHECK_CONFIGURE_FLAGS = \
	--with-systemdsystemunitdir=$$dc_install_base/$(systemdunitdir)

systemdunitdir = @SYSTEMD_UNITDIR@

systemdunit_DATA = \
	data/swupd-update.service \
	data/swupd-update.timer \
	data/verifytime.service

TEST_EXTENSIONS = .bats .test

if ENABLE_TESTS
tap_driver = env AM_TAP_AWK='$(AWK)' $(SHELL) \
	$(top_srcdir)/tap-driver.sh

BATS_LOG_DRIVER = $(tap_driver)

TESTS = \
	test/unit/test_list.test \
	test/unit/test_manifest.test \
	test/unit/test_signature.test \
	test/unit/test_strings.test \
	test/unit/test_sys.test

EXTRA_DIST += test/unit/test_helper.h test/unit/data

check_PROGRAMS = $(TESTS)
LDADD = $(swupd_LDADD) $(swupd_OBJECTS:src/main.o=)

endif

if ENABLE_MANPAGE
MANPAGES = \
	docs/swupd.1 \
	docs/swupd-alias.7 \
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
	test/code_analysis/warning_functions.bats
	test/code_analysis/quiet_output.bats
	scripts/flag_validator.bash

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
	env TESTS="$(shell find test/functional  -name *.bats \( ! -path "*only_in_ci_system*" \) | sort)" $(MAKE) -e check
	env TESTS="$(shell find ./test/functional/only_in_ci_system  -name *.bats | sort)" $(MAKE) -e -j1 check

docs-coverage:
	doxygen docs/Doxyfile || die; \
	python -m coverxygen --xml-dir xml/ --src-dir ./ --output doc-coverage.info --exclude ".*/src/swupd_lib/globals.h" --exclude ".*/src/swupd_build_opts.h" --exclude ".*/src/swupd_build_variant.h" --exclude ".*/src/swupd.h" --exclude ".*/src/swupd_lib/manifest.h" --exclude ".*/src/swupd_lib/swupd_curl_internal.h" --exclude ".*/src/swupd_lib/swupd_curl.h" --format json || die; \
	if [ "$$(cat doc-coverage.info | grep documented.*false | wc -l)" -gt "0" ]; then \
		echo "Public functions or data added on headers but not documented on src/lib "; \
		exit 1; \
	fi

endif
