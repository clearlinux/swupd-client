#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

setup_file() {

	create_test_environment "$TEST_NAME"

	small=/small-file:"$(create_file "$WEB_DIR"/10/files 1MB)"
	medium=/medium-file:"$(create_file "$WEB_DIR"/10/files 2MB)"
	large=/large-file:"$(create_file "$WEB_DIR"/10/files 3MB)"

	create_bundle -n alfa-zulu -f /yankee/alfalfa,/alfaromeo/echo,/foo/quebec,/foo/alfa,/foo/delta,/bar/alfa,"$small" "$TEST_NAME"
	create_bundle -n victor -f /yankee,/bar/alfa,/alfa/zulu,/papa,"$large" "$TEST_NAME"
	create_bundle -n alfa-charly -f /foxtrot,/tango/sierra,/kilo,"$medium" "$TEST_NAME"

}

teardown_file() {

	destroy_test_environment --force "$TEST_NAME"

}

@test "SRH023: search shows the results in alphabetical order" {

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS --order=alpha alfa"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Bundle alfa-charly \\(2 MB to install\\)
		./usr/share/clear/bundles/alfa-charly
		Bundle alfa-zulu \\(1 MB to install\\)
		./alfaromeo/echo
		./bar/alfa
		./foo/alfa
		./usr/share/clear/bundles/alfa-zulu
		./yankee/alfalfa
		Bundle victor \\(3 MB to install\\)
		./alfa/zulu
		./bar/alfa
	EOM
	)
	assert_regex_in_output "$expected_output"

}

@test "SRH024: search shows the results ordered by bundle size (smallest first)" {

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS --order=size alfa"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Bundle alfa-zulu \\(1 MB to install\\)
		./usr/share/clear/bundles/alfa-zulu
		./bar/alfa
		./foo/alfa
		./alfaromeo/echo
		./yankee/alfalfa
		Bundle alfa-charly \\(2 MB to install\\)
		./usr/share/clear/bundles/alfa-charly
		Bundle victor \\(3 MB to install\\)
		./alfa/zulu
		./bar/alfa
	EOM
	)
	assert_regex_in_output "$expected_output"

}

@test "SRH025: search can show all files in all bundles" {

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Downloading all Clear Linux manifests
		Searching for ''
		Bundle alfa-charly .*
		./usr/share/clear/bundles/alfa-charly
		./medium-file
		./kilo
		./tango/sierra
		./foxtrot
		Bundle alfa-zulu .*
		./usr/share/clear/bundles/alfa-zulu
		./small-file
		./bar/alfa
		./foo/delta
		./foo/alfa
		./foo/quebec
		./alfaromeo/echo
		./yankee/alfalfa
		Bundle os-core .*
		./usr/share/clear/bundles/os-core
		./core
		Bundle victor .*
		./usr/share/clear/bundles/victor
		./large-file
		./papa
		./alfa/zulu
		./bar/alfa
		./yankee
	EOM
	)
	assert_regex_in_output "$expected_output"

}

@test "SRH026: search limits the results" {

	run sudo sh -c "$SWUPD search-file $SWUPD_OPTS --top=3 alfa"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Searching for 'alfa'
		Displaying only top 3 file results per bundle
		File results may be truncated
		Bundle alfa-charly \\(2 MB to install\\)
		./usr/share/clear/bundles/alfa-charly
		Bundle alfa-zulu \\(1 MB to install\\)
		./usr/share/clear/bundles/alfa-zulu
		./bar/alfa
		./foo/alfa
		Bundle victor \\(3 MB to install\\)
		./alfa/zulu
		./bar/alfa
	EOM
	)
	assert_regex_in_output "$expected_output"

}
#WEIGHT=6
