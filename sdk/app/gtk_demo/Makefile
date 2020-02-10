HOME_TREE := ../../
  
MODULE_NAME := $(notdir $(CURDIR))

include $(HOME_TREE)/mak_def.inc

PKGCONFIG := PKG_CONFIG_PATH=PKG_CONFIG_PATH:/opt/local/lib/pkgconfig:/opt/local/share/pkgconfig pkg-config

CXXSRC += main.cpp

C_INCLUDES += `$(PKGCONFIG) --cflags gtk+-3.0` -I$(CURDIR)/../../sdk/include -I$(CURDIR)/../../sdk/src

EXTRA_OBJ :=
LD_LIBS += -lstdc++ -lm -lpthread `$(PKGCONFIG) --libs gtk+-3.0`

all: build_app

include $(HOME_TREE)/mak_common.inc

clean: clean_app

