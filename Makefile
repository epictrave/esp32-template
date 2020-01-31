#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#
VERSION := 0.0.10

PROJECT_NAME := $(subst _,-,${DeviceID})
EXTRA_COMPONENT_DIRS += ${PROJECT_PATH}/components
PROJECT_VER = $(VERSION)

include $(IDF_PATH)/make/project.mk
include sdkconfig

CFLAGS+= -D${DeviceID}
CFLAGS+= -DVERSION=\"$(VERSION)\"
CFLAGS+= -Wno-error=maybe-uninitialized 