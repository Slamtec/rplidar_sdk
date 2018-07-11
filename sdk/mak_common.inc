#/*
# *  RPLIDAR SDK
# *
# *  Copyright (c) 2009 - 2014 RoboPeak Team
# *  http://www.robopeak.com
# *  Copyright (c) 2014 - 2018 Shanghai Slamtec Co., Ltd.
# *  http://www.slamtec.com
# *
# */
#/*
# * Redistribution and use in source and binary forms, with or without
# * modification, are permitted provided that the following conditions are met:
# *
# * 1. Redistributions of source code must retain the above copyright notice,
# *    this list of conditions and the following disclaimer.
# *
# * 2. Redistributions in binary form must reproduce the above copyright notice,
# *    this list of conditions and the following disclaimer in the documentation
# *    and/or other materials provided with the distribution.
# *
# * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
# * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# *
# */

#  Common Generation Law for rplidar sdk Building
#
#
#

.PHONY: make_subs build_sdk build_app clean_sdk clean_app

make_subs:
	@for subdir in $(MAKE_TARGETS) ; do  $(MAKE) -C $$subdir $(MAKECMDGOALS) || exit 1;  done

# Define all object files.

_CSRC   = $(filter-out %*.c,$(CSRC))
_CXXSRC = $(filter-out %*.cpp,$(CXXSRC))
_ASRC   = $(filter-out %*.S,$(ASRC))

OBJ += $(patsubst %.c, $(TARGET_OBJ_ROOT)/%.o, $(_CSRC))
OBJ += $(patsubst %.cpp, $(TARGET_OBJ_ROOT)/%.o, $(_CXXSRC))
OBJ += $(patsubst %.S, $(TARGET_OBJ_ROOT)/%.o, $(_ASRC))

build_sdk:  $(SDK_TARGET)

clean_sdk:
	$(RMDIR) $(TARGET_OBJ_ROOT)
	$(RM) $(SDK_TARGET)

build_app: $(APP_TARGET)

clean_app:
	$(RMDIR) $(TARGET_OBJ_ROOT)
	$(RM) $(APP_TARGET)

$(SDK_TARGET): $(OBJ) $(EXTRA_OBJ)
	$(MKDIR) `dirname $@`
	@for i in $^; do echo " pack `basename $$i`->`basename $@`"; $(AR) rcs $@ $$i; done
	
$(APP_TARGET): $(OBJ) $(EXTRA_OBJ) $(SDK_TARGET)
	@$(MKDIR) `dirname $@`
	@echo " LD   $@"
	$(CMD_HIDE)$(CC) $(ALL_CXXFLAGS) -o $@ $^ $(LDFLAGS)  


#
# Compile: create object files from C++ source files.
$(TARGET_OBJ_ROOT)/%.o: %.cpp
	@$(MKDIR) `dirname $@`
	@echo " CXX  $<"
	$(CMD_HIDE)$(CXX) -c $(ALL_CXXFLAGS) $< -o $@ 

# Compile: create object files from C source files.
$(TARGET_OBJ_ROOT)/%.o: %.c
	@$(MKDIR) `dirname $@`
	@echo " CC   $<"
	$(CMD_HIDE)$(CC) -c $(ALL_CFLAGS) $< -o $@ 

# Assemble: create object files from assembler source files.
$(TARGET_OBJ_ROOT)/%.o: %.S
	@$(MKDIR) `dirname $@`
	@echo " AS   $<"
	$(CMD_HIDE)$(CC) -c $(ALL_ASFLAGS) $< -o $@

