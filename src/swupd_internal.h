#ifndef __SWUPD_INTERNAL__
#define __SWUPD_INTERNAL__

#ifdef __cplusplus
extern "C" {
#endif

enum swupd_code autoupdate_main(int argc, char **argv);
enum swupd_code bundle_add_main(int argc, char **argv);
enum swupd_code bundle_remove_main(int argc, char **argv);
enum swupd_code bundle_list_main(int argc, char **argv);
enum swupd_code hashdump_main(int argc, char **argv);
enum swupd_code update_main(int argc, char **argv);
enum swupd_code check_update_main(int argc, char **argv);
enum swupd_code search_file_main(int argc, char **argv);
enum swupd_code info_main(int argc, char **argv);
enum swupd_code clean_main(int argc, char **argv);
enum swupd_code mirror_main(int argc, char **argv);
enum swupd_code binary_loader_main(int argc, char **argv);
enum swupd_code install_main(int argc, char **argv);
enum swupd_code repair_main(int argc, char **argv);
enum swupd_code diagnose_main(int argc, char **argv);
enum swupd_code bundle_info_main(int argc, char **argv);
enum swupd_code verify_main(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif
