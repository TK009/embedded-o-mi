
TARGET_ARCH =
BASEFLAGS = `pkg-config --cflags check`
TESTFLAGS = -fprofile-instr-generate -fcoverage-mapping -fsanitize=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope
DEBUGFLAGS ?= $(TESTFLAGS) -g -Wall -Wextra -Wno-gnu-statement-expression -pedantic -Wno-empty-translation-unit -Wno-gnu-folding-constant
ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1
#-Wno-reorder
OPTIMIZE = #-O3 -flto


CXXSTD = -std=c++14 
CSTD = -std=c11 #-D_POSIX_C_SOURCE=200809L


#CXX    = g++
CC     = @clang
rm     = rm -f
GDB    = CK_FORK=no gdb

# change these to set the proper directories where each files shoould be
SRCDIR  := src
OBJDIR  := obj
BINDIR  := bin
TESTDIR := test

vpath $(SRCDIR) $(TESTDIR)

#BASEDIR = $(SRCDIR)/base
#FWDIRS  = $(sort $(dir $(wildcard $(FW)*/)))
SRCS = $(shell find $(SRCDIR)/ -name '*.c')
TESTSRCS = $(shell find $(TESTDIR)/ -name '*.check')
OBJS := $(addsuffix .o,$(basename $(SRCS)))
OBJS := $(OBJS:$(SRCDIR)/%=$(OBJDIR)/%)
#TESTOBJS = $(addsuffix .o,$(basename $(TESTSRCS)))
#TESTOBJS := $(TESTOBJS:$(TESTDIR)/%=$(OBJDIR)/%)
TESTBINARIES = $(addsuffix .test,$(basename $(TESTSRCS)))
TESTBINARIES := $(TESTBINARIES:$(TESTDIR)/%=$(OBJDIR)/%)
TESTRESULTS = $(addsuffix .log,$(basename $(TESTBINARIES)))
TESTDATA = $(addsuffix .profraw,$(basename $(TESTBINARIES)))


HEADERS = -I$(SRCDIR)
CXXFLAGS = $(CXXSTD) $(HEADERS) $(DEBUGFLAGS) $(OPTIMIZE)
CFLAGS = $(CSTD) $(HEADERS) $(DEBUGFLAGS) $(OPTIMIZE)



LDFLAGS = # -static-libstdc++
LDTESTFLAGS = `pkg-config --libs check` -fprofile-instr-generate -fcoverage-mapping

#OBJECTS  := 

EXECUTABLES :=  # $(BINDIR)/app


# TODO
all: $(TESTBINARIES)
	
$(EXECUTABLES): | $(BINDIR)


$(BINDIR):
	mkdir -p $@
$(OBJDIR):
	mkdir -p $@

$(OBJDIR)/%.check.c: $(TESTDIR)/%.check | $(OBJDIR)
	checkmk $< > $@


# DEPS
DEPDIR := $(OBJDIR)
#.d

# always create dir?
#$(shell mkdir -p $(DEPDIR) >/dev/null)

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
#COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = @mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

%.o : %.c
#%.o : %.c $(DEPDIR)/%.d
$(OBJDIR)/%.check.o: $(OBJDIR)/%.check.c $(DEPDIR)/%.d
	@echo C $@
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPDIR)/%.d | $(OBJDIR)
	@echo C $@
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

%.d: ;

.PRECIOUS: %.d $(DEPDIR)/%.d $(OBJDIR)/%.check.o $(OBJDIR)/%.check.c $(OBJDIR)/%.log $(OBJDIR)/%.o

#$(TESTBINARIES)
$(OBJDIR)/%.test: $(OBJDIR)/%.check.o $(OBJS)
	@echo
	@echo "LINKING $@!"
	$(CC) -o $@ $(LDTESTFLAGS) $(DEBUGFLAGS) $^
#@echo "LINKING $@ complete!"




#$(OBJDIR)/%.o: $(SRCDIR)/%.c Makefile
#	@mkdir -p $(@D)
#	$(CC) $(MYCFLAGS) -c $< -o $@
#	@$(CC) $(MYCFLAGS) -MM $< -MF $(OBJDIR)/$*.d # dependencies
#	@sed -i '1s|^.*:|$@:|' $(OBJDIR)/$*.d # fix target path 

.PHONEY: clean debug test coverageclean coverage
clean:
	$(rm) -rf $(OBJDIR)
	$(rm) default.profraw
	@echo "Cleanup complete!"

SHELL=/bin/bash -o pipefail

# Run tests, maybe print backtrace, run valgrind
$(OBJDIR)/%.log: $(OBJDIR)/%.test
	@echo -e "\nRUNNING TEST $<"
	@(LLVM_PROFILE_FILE="$(basename $<).profraw" ./$< | tee $@) || (ASAN_OPTIONS=detect_leaks=0 $(GDB) -q -ex run -ex bt -ex "kill inferiors 1" -ex quit $<; exit 1)

#@echo -e "\nRUNNING VALGRIND" | tee -a $@
#CK_FORK=no valgrind -q --leak-check=full $< >> $@


test: $(TESTRESULTS)
	@echo
	@llvm-profdata merge -sparse $(TESTDATA) -o $(OBJDIR)/default.profdata
	@llvm-cov report -use-color -instr-profile=$(OBJDIR)/default.profdata $(OBJDIR)/odf.test $(addprefix "-object=", $(TESTBINARIES)) | sed 's/-----------------------------------------//'

coverage:
	@llvm-cov show $(TESTBINARIES) -instr-profile ./obj/default.profdata

coverageclean:
	@rm -f $(OBJDIR)/*.prof{raw,data}

.DELETE_ON_ERROR: %.o %.log

-include $(OBJS:.o=.d)
#include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))
