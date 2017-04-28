#ifndef __INCLUDE_GUARD_SWUPD_INTERNAL_H
#define __INCLUDE_GUARD_SWUPD_INTERNAL_H

extern int autoupdate_main(int argc, char **argv);
extern int bundle_add_main(int argc, char **argv);
extern int bundle_remove_main(int argc, char **argv);
extern int bundle_list_main(int argc, char **argv);
extern int hashdump_main(int argc, char **argv);
extern int update_main(int argc, char **argv);
extern int verify_main(int argc, char **argv);
extern int check_update_main(int argc, char **argv);
extern int search_main(int argc, char **argv);

#endif
