
.PHONY: test clean style flush_turds base show_sde_cmd

TARGET ?= icl

CC=gcc
SDE ?= sde64

ASTYLE_OPTS ?= -s4 -xC100 -xt2 -K -S -p -xg -f -c

TUNE_skx = -mtune=skylake-avx512
TUNE_cnl = -mtune=cannonlake
TUNE_icl = -mtune=icelake-client

OPTS_skx = -mavx512f -mavx512dq -mavx512bw -mavx512vl -mavx512cd
OPTS_cnl = $(OPTS_skx) -mavx512vbmi -mavx512ifma
OPTS_icl = $(OPTS_cnl) -mavx512vbmi2 -mvpclmulqdq -mgfni -mvaes -mavx512vpopcntdq

wantlist=$(sort $(subst -m,,$(filter-out -march=% -mtune=% , $(OPTS_$(TARGET)))))
havelist=$(sort $(shell egrep '^flags\s*:' /proc/cpuinfo | head -n1 | cut -d ':' -f 2- | xargs -n1 echo | egrep "^$$(echo $(wantlist) | tr ' ' '|')$$"))

ifeq "$(wantlist)" "$(havelist)"
SDE_CMD ?=
else
SDE_CMD ?= $(SDE) -$(TARGET) --
endif

ifdef DEBUG
	CFLAGS_BASE = -O0
else
	CFLAGS_BASE = -O3
endif

show_sde_cmd:
	@echo -n "sde command is: "
	@echo $(SDE_CMD)

CFLAGS = -g -Wall $(TUNE_$(TARGET)) $(OPTS_$(TARGET)) $(CFLAGS_BASE)

test_srcs = test/*.c

base_objs = src/base_util.o

# While I _could_ make an auto-deps mechanism for the one or two
# object files so that touching the headers would make it rebuild
# the objects, that's just plain silly given this is nearly ALL
# headers anyway (as intended).  As such I just always clean.

all: clean base

base: $(base_objs)

.ONESHELL: test
test: $(base_objs) $(test_srcs)
	@echo "Starting tests using \"$(SDE_CMD)\" to run tests:"
	@rm -f test/a.out
	@set -e pipefail
	@for f in $(filter-out $(base_objs),$^); do
	@    $(CC) $(CFLAGS) -o test/a.out $(base_objs) $$f || exit $$?
	@    $(SDE_CMD) test/a.out || exit $$?
	@    rm -f test/a.out;
	@done
	@echo "All tests PASS."


clean:
	@echo "Cleaning."
	@rm -f src/*.o
	
style:
	find . -type f -name "*.[ch]" | xargs astyle $(ASTYLE_OPTS)

flush_turds:
	find . -type f -name "*.orig" | xargs rm -f
	find . -type f -name "*.rej" | xargs rm -f
	find . -type f -name "*~" | xargs rm -f

