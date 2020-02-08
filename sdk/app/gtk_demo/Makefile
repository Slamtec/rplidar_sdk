HOME_TREE := ../../

MODULE_NAME := $(notdir $(CURDIR))

include $(HOME_TREE)/mak_def.inc

CXXSRC += main.cpp
C_INCLUDES += `pkg-config --cflags gtk+-3.0` -I$(CURDIR)/../../sdk/include -I$(CURDIR)/../../sdk/src

EXTRA_OBJ :=
LD_LIBS += -lstdc++ -lpthread `pkg-config --libs gtk+-3.0`

all: build_app

include $(HOME_TREE)/mak_common.inc

clean: clean_app

