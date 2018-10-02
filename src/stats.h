#ifndef __INCLUDE_GUARD_SWUPD_INTERNAL_H
#define __INCLUDE_GUARD_SWUPD_INTERNAL_H

enum swupd_statistics {
	DELETED_FILES,
	CHANGED_FILES,
	NEW_BUNDLES,
	DELETED_BUNDLES,
	CHANGED_BUNDLES,
	STATISTICS_SIZE
};

extern int swupd_stats[];

static inline void account_deleted_file(void)
{
	swupd_stats[DELETED_FILES]++;
}

static inline void account_changed_file(void)
{
	swupd_stats[CHANGED_FILES]++;
};

static inline void account_new_bundle(void)
{
	swupd_stats[NEW_BUNDLES]++;
};

static inline void account_deleted_bundle(void)
{
	swupd_stats[DELETED_BUNDLES]++;
};

static inline void account_changed_bundle(void)
{
	swupd_stats[CHANGED_BUNDLES]++;
};

extern void print_statistics(int version1, int version2);

#endif
