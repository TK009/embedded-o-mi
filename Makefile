
TARGET_ARCH =
BASEFLAGS = `pkg-config --cflags check`
TESTFLAGS = -fprofile-instr-generate -fcoverage-mapping -fsanitize=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope
DEBUGFLAGS ?= $(TESTFLAGS) -g -Wall -Wextra -Wno-gnu-statement-expression -pedantic -Wno-empty-translation-unit -Wno-gnu-folding-constant
ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1
#-Wno-reorder
OPTIMIZE = #-O3 -flto


CXXSTD = -std=c++14 
CSTD = -std=c11 # -D_POSIX_C_SOURCE=200809L


#CXX    = g++
CC     = @clang
rm     = rm -f
GDB    = ASAN_OPTIONS=detect_leaks=0 CK_FORK=no gdb

# change these to set the proper directories where each files shoould be
SRCDIR  := src
OBJDIR  := obj
BINDIR  := bin
TESTDIR := test

vpath $(SRCDIR) $(TESTDIR)

#BASEDIR = $(SRCDIR)/base
#FWDIRS  = $(sort $(dir $(wildcard $(FW)*/)))
SRCS = $(shell find $(SRCDIR)/ -name '*.c')
SRCS += $(shell find $(SRCDIR)/ -name '*.py')
TESTSRCS = $(shell find $(TESTDIR)/ -name '*.check')
OBJS := $(addsuffix .o,$(basename $(SRCS)))
OBJS := $(OBJS:$(SRCDIR)/%=$(OBJDIR)/%)
TESTBINARIES = $(addsuffix .test,$(basename $(TESTSRCS)))
TESTBINARIES := $(TESTBINARIES:$(TESTDIR)/%=$(OBJDIR)/%)
TESTOBJS := $(addsuffix .check.o,$(basename $(TESTBINARIES)))
TESTRESULTS = $(addsuffix .log,$(basename $(TESTBINARIES)))
TESTDATA = $(addsuffix .profraw,$(basename $(TESTBINARIES)))


HEADERS = -iquote $(SRCDIR) -iquote $(OBJDIR)
#HEADERS = -I $(SRCDIR) -I $(OBJDIR)
CXXFLAGS = $(CXXSTD) $(HEADERS) $(DEBUGFLAGS) $(OPTIMIZE)
CFLAGS = $(CSTD) $(HEADERS) $(DEBUGFLAGS) $(OPTIMIZE)



LDFLAGS = # -static-libstdc++
LDTESTFLAGS = `pkg-config --libs check` -fprofile-instr-generate -fcoverage-mapping

#OBJECTS  := 

EXECUTABLES :=  # $(BINDIR)/app



# TODO: embedded compilation
all: $(TESTBINARIES)
	
$(EXECUTABLES): | $(BINDIR)


$(BINDIR):
	mkdir -p $@
$(OBJDIR):
	mkdir -p $@



# Code generators:

$(OBJDIR)/%.check.c: $(TESTDIR)/%.check | $(OBJDIR)
	checkmk $< > $@

$(OBJDIR)/%.c: $(SRCDIR)/%.py
	./$< c > $@
$(OBJDIR)/%.h: $(SRCDIR)/%.py
	./$< h > $@




# DEPS, FIXME: hardcoded to $(OBJDIR) by using $@
DEPDIR := $(OBJDIR)
#.d

COVERAGEDIR ?= $(OBJDIR)

# always create dir?
#$(shell mkdir -p $(DEPDIR) >/dev/null)

# using $(@:.o=.Td) instead of $(DEPDIR)/$*.Td because of obj/%.check.o pattern misses ".check" part
DEPFLAGS = -MT $@ -MMD -MP -MF $(@:.o=.Td)

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
#COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

# using $(@:.o=.Td) instead of $(DEPDIR)/$*.Td because of obj/%.check.o pattern misses ".check" part
POSTCOMPILE = @mv -f $(@:.o=.Td) $(@:.o=.d)
# && touch $@

%.o : %.c
#%.o : %.c $(DEPDIR)/%.d
$(OBJDIR)/%.o: $(OBJDIR)/%.c $(DEPDIR)/%.d
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

.PHONEY: clean debug test coverageclean coverage coverage-html
clean:
	$(rm) -rf $(OBJDIR)
	$(rm) default.profraw
	@echo "Cleanup complete!"

SHELL=/bin/bash -o pipefail

# Run tests, maybe print backtrace, run valgrind
$(OBJDIR)/%.log: $(OBJDIR)/%.test
	@echo -e "\nRUNNING TEST $<"
	@(LLVM_PROFILE_FILE="$(basename $<).profraw" ./$< | tee $@) || ($(GDB) -q -ex run -ex bt -ex "kill inferiors 1" -ex quit $<; exit 1)

#@echo -e "\nRUNNING VALGRIND" | tee -a $@
#CK_FORK=no valgrind -q --leak-check=full $< >> $@

tags: $(SRCS)
	ctags $(SRCDIR)

NEWESTSOURCE="${SRCDIR}/$(shell ls -t ${SRCDIR} | head -1)" 

test: $(TESTRESULTS) tags
	@echo
	@llvm-profdata merge -sparse $(TESTDATA) -o $(OBJDIR)/default.profdata
	@llvm-cov report --use-color -ignore-filename-regex='yxml*' --instr-profile=$(OBJDIR)/default.profdata $(OBJDIR)/odf.test $(addprefix "--object=", $(TESTBINARIES)) | sed 's/-----------------------------------------//'
	@echo
	@echo "Some uncovered regions (search for 0 count lines if no red) in $(NEWESTSOURCE):"
	@llvm-cov show $(TESTBINARIES) --instr-profile ./obj/default.profdata $(NEWESTSOURCE) --use-color --show-expansions --show-line-counts-or-regions | egrep --color=never '(\[0;41m|   \^?0)' -C 3 | head -n 25 || true

coverage:
	@llvm-cov show $(TESTBINARIES) --instr-profile ./obj/default.profdata

coverage-html: ${COVERAGEDIR}
	@llvm-cov show $(TESTBINARIES) --instr-profile ./obj/default.profdata --format=html --show-expansions --output-dir=${COVERAGEDIR}
	@echo Full coverage report: file://$(abspath ${COVERAGEDIR})/index.html

#@xdg-open file://$(abspath ${COVERAGEDIR})/index.html

coverageclean:
	@rm -f $(OBJDIR)/*.prof{raw,data} ${COVERAGEDIR}

debug:
	$(GDB) $(ARGS)

.DELETE_ON_ERROR: %.o %.log

info:
	@echo $(TESTOBJS)

-include $(OBJS:.o=.d)
-include $(TESTOBJS:.o=.d)
#include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))
