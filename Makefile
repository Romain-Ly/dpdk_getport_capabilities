ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc
RTE_OUTPUT = bin
RTE_TOOLCHAIN = gcc

include $(RTE_SDK)/mk/rte.vars.mk

# binary name
APP = getport_capabilities

# all source are stored in SRCS-y
SRCS-y := src/main.c
SRCS-y += src/getport_capabilities.c

SRCS-y += $(RTE_SRCDIR)/../dpdk_getflags_name/src/getflags_name.c
CFLAGS += -I$(RTE_SRCDIR)/../dpdk_getflags_name/include






CFLAGS += -I$(RTE_SRCDIR)/include
CFLAGS += -std=gnu11
CFLAGS += -O0
CFLAGS += -Wall
CFLAGS += -g 
CFLAGS += $(WERROR_FLAGS)


include $(RTE_SDK)/mk/rte.extapp.mk

