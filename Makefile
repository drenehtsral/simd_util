
.PHONY: test clean

TARGET ?= icl

CC=gcc-8
SDE ?= sde64

SDE_CMD ?= $(SDE) -$(TARGET) --

OPTS_skx = -mavx512f -mavx512dq -mavx512bw -mavx512vl -mavx512cd
OPTS_icl = $(OPTS_skx) -mavx512vpopcntdq -mavx512vbmi

#fixme -- Newer binutils than ubuntu comes with?
OPTS_broken = -mavx512vbmi2 -mvpclmulqdq

ifdef DEBUG
	CFLAGS_BASE = -g -O0
else
	CFLAGS_BASE = -O3
endif

CFLAGS = -Wall $(OPTS_$(TARGET)) $(CFLAGS_BASE)

test_srcs = test/*.c

base_objs = src/base_util.o


all: base


base: $(base_objs)

test: $(base_objs) $(test_srcs)
	@echo "Starting tests using \"$(SDE_CMD)\" to run tests:"
	@for f in $(filter-out $(base_objs),$^); do $(CC) $(CFLAGS) -o test/a.out $(base_objs) $$f &&  $(SDE_CMD) test/a.out && rm -f test/a.out; done
	@echo "All tests PASS."


clean:
	rm -f src/*.o
	

