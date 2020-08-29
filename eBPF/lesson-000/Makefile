BPFCODE = bpf_program

KERNEL_SRC = ./linux-5.3
BPFLOADER = $(KERNEL_SRC)/samples/bpf/bpf_load.c

CCINCLUDE += -I$(KERNEL_SRC)/tools/testing/selftests/bpf  # Include bpf_helpers.h

LOADINCLUDE += -I$(KERNEL_SRC)/samples/bpf
LOADINCLUDE += -I$(KERNEL_SRC)/tools/lib
LOADINCLUDE += -I$(KERNEL_SRC)/tools/perf
LOADINCLUDE += -I$(KERNEL_SRC)/tools/include
LIBRARY_PATH = -L/usr/lib
BPFSO = -lbcc_bpf

.PHONY: bpfload build

all: bpfload

build: ${BPFCODE.c} ${BPFLOADER}
	clang -O2 -target bpf -c $(BPFCODE:=.c) $(CCINCLUDE) -o ${BPFCODE:=.o}

bpfload: build
	clang -o bpf-program -lelf $(LOADINCLUDE) $(LIBRARY_PATH) $(BPFSO) $(BPFLOADER) loader.c
