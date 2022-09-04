HEAP_SIZE	= 8388208
STACK_SIZE	= 61800

PRODUCT = Skeleton.pdx

# Locate the SDK
SDK = ${PLAYDATE_SDK_PATH}
ifeq ($(SDK),)
SDK = $(shell egrep '^\s*SDKRoot' ~/.Playdate/config | head -n 1 | cut -c9-)
endif

ifeq ($(SDK),)
$(error SDK path not found; set ENV value PLAYDATE_SDK_PATH)
endif

VPATH += Source

# List C source files here
SRC =	\
		main.cpp \

# List all user directories here
UINCDIR =

# List user asm files
UASRC =

# List all user C define here, like -D_DEBUG=1
UDEFS =

# Define ASM defines here
UADEFS =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

include $(SDK)/C_API/buildsupport/common.mk

# Tweaks for C++
#
# In $(SDK)/C_API/buildsupport/common.mk
# Comment out this line:
#	_OBJS	= $(SRC:.c=.o)
# Add this line:
#	_OBJS = $(patsubst %.cpp,%.cpp.o,$(patsubst %.c,%.o,$(SRC)))
GCC:=$(SDK)/../arm-gnu-toolchain-11.3.rel1-darwin-x86_64-arm-none-eabi/bin/
CPP = $(GCC)$(TRGT)g++ -g -std=c++20 -fno-rtti -fno-exceptions -fpermissive

$(OBJDIR)/%.cpp.o : %.cpp | OBJDIR DEPDIR
	mkdir -p `dirname $@`
	$(CPP) -c $(CPFLAGS) -I . $(INCDIR) $< -o $@
