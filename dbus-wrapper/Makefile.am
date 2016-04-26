bin_PROGRAMS = swupdd swupdctl

swupdd_SOURCES = \
	list.c \
	swupdd-main.c \
	$(NULL)

swupdd_CFLAGS = \
	-Wall \
	$(SWUPDD_CFLAGS) \
	$(NULL)

swupdd_LDADD = \
	$(SWUPDD_LIBS) \
	$(NULL)

swupdctl_SOURCES = \
	swupdctl.c \
	helpers.c \
	list.c \
	dbus_client.c \
	option.c \
	cmd_bundle_add.c \
	cmd_bundle_remove.c \
	cmd_hashdump.c \
	cmd_verify.c \
	cmd_update.c \
	cmd_check_update.c \
	cmd_search.c \
	$(NULL)

swupdctl_CFLAGS = \
	-Wall \
	$(SWUPDD_CFLAGS) \
	$(NULL)

swupdctl_LDADD = \
	$(SWUPDD_LIBS) \
	$(NULL)
