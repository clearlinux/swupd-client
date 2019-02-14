#ifndef __INCLUDE_GUARD_SWUPD_INTERNAL_H
#define __INCLUDE_GUARD_SWUPD_INTERNAL_H

extern enum swupd_code autoupdate_main(int argc, char **argv);
extern enum swupd_code bundle_add_main(int argc, char **argv);
extern enum swupd_code bundle_remove_main(int argc, char **argv);
extern enum swupd_code bundle_list_main(int argc, char **argv);
extern enum swupd_code hashdump_main(int argc, char **argv);
extern enum swupd_code update_main(int argc, char **argv);
extern enum swupd_code verify_main(int argc, char **argv);
extern enum swupd_code check_update_main(int argc, char **argv);
extern enum swupd_code search_main(int argc, char **argv);
extern enum swupd_code info_main(int argc, char **argv);
extern enum swupd_code clean_main(int argc, char **argv);
extern enum swupd_code mirror_main(int argc, char **argv);
extern enum swupd_code binary_loader_main(int argc, char **argv);

#endif
