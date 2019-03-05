#!/usr/bin/env bats

# Author: Castulo Martinez
# Email: castulo.martinez@intel.com

load "../testlib"

global_setup() {

	create_test_environment "$TEST_NAME"

	small=/small-file:"$(create_file "$WEBDIR"/10/files 1MB)"
	medium=/medium-file:"$(create_file "$WEBDIR"/10/files 2MB)"
	large=/large-file:"$(create_file "$WEBDIR"/10/files 3MB)"

	create_bundle -n alfa-zulu -f /yankee/alfalfa,/alfaromeo/echo,/foo/quebec,/foo/alfa,/foo/delta,/bar/alfa,"$small" "$TEST_NAME"
	create_bundle -n victor -f /yankee,/bar/alfa,/alfa/zulu,/papa,"$large" "$TEST_NAME"
	create_bundle -n alfa-charly -f /foxtrot,/tango/sierra,/kilo,"$medium" "$TEST_NAME"

}

test_setup() {

	# do nothing
	return

}

test_teardown() {

	# do nothing
	return

}

global_teardown() {

	destroy_test_environment "$TEST_NAME"

}

@test "SRH023: search shows the results in alphabetical order" {

	run sudo sh -c "$SWUPD search-legacy $SWUPD_OPTS -T 10 alfa"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Bundle alfa-charly	\\(2 MB to install\\)
		./usr/share/clear/bundles/alfa-charly
		Bundle alfa-zulu	\\(1 MB to install\\)
		./alfaromeo/echo
		./bar/alfa
		./foo/alfa
		./usr/share/clear/bundles/alfa-zulu
		./yankee/alfalfa
		Bundle victor	\\(3 MB to install\\)
		./alfa/zulu
		./bar/alfa
	EOM
	)
	assert_regex_in_output "$expected_output"

}

@test "SRH024: search shows the results ordered by bundle size (smallest first)" {

	run sudo sh -c "$SWUPD search-legacy $SWUPD_OPTS alfa"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Bundle alfa-zulu	\\(1 MB to install\\)
		./usr/share/clear/bundles/alfa-zulu
		./bar/alfa
		./foo/alfa
		./alfaromeo/echo
		./yankee/alfalfa
		Bundle alfa-charly	\\(2 MB to install\\)
		./usr/share/clear/bundles/alfa-charly
		Bundle victor	\\(3 MB to install\\)
		./alfa/zulu
		./bar/alfa
	EOM
	)
	assert_regex_in_output "$expected_output"

}

@test "SRH025: search can show all files in all bundles" {

	run sudo sh -c "$SWUPD search-legacy $SWUPD_OPTS -d"

	assert_status_is 0
	expected_output=$(cat <<-EOM
		Displaying all bundles and their files:
		--Bundle: alfa-charly--
		    /usr/share/clear/bundles/alfa-charly
		    /medium-file
		    /kilo
		    /tango/sierra
		    /foxtrot
		--Bundle: victor--
		    /usr/share/clear/bundles/victor
		    /large-file
		    /papa
		    /alfa/zulu
		    /bar/alfa
		    /yankee
		--Bundle: alfa-zulu--
		    /usr/share/clear/bundles/alfa-zulu
		    /small-file
		    /bar/alfa
		    /foo/delta
		    /foo/alfa
		    /foo/quebec
		    /alfaromeo/echo
		    /yankee/alfalfa
		--Bundle: os-core--
		    /usr/share/clear/bundles/os-core
		    /core
	EOM
	)
	assert_regex_is_output "$expected_output"

}