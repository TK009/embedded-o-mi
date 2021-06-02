

#ESP32S2JS := $(LIBDIR)/esp32s2/libjerry-core.a $(LIBDIR)/esp32s2/libjerry-ext.a
#BASEFLAGS = 
SANITIZER ?= address
ifeq ($(SANITIZER),address)
SANITIZER_OPT = -fsanitize=address -fsanitize-address-use-after-scope -fsanitize=undefined
CHECKLIB=`pkg-config --libs check`
COVERAGE = -fprofile-instr-generate -fcoverage-mapping -fno-omit-frame-pointer 

else ifeq ($(SANITIZER),memory)
SANITIZER_OPT = -fsanitize=memory -fno-optimize-sibling-calls -fsanitize-memory-track-origins=2 -fsanitize-blacklist=sanignore.txt #-fsanitize-ignorelist=sanignore.txt
# check has a problem with memory san (fwrite in ppack), needs to be compiled with the ignore list:
# wget https://github.com/libcheck/check/releases/download/0.15.2/check-0.15.2.tar.gz
# tar xf check-0.15.2.tar.gz
# mv check-0.15.2 check
# ln -s ../sanignore.txt check/
# ln -s ../../sanignore.txt check/src/
# ln -s ../../sanignore.txt check/lib/
# ln -s ../../sanignore.txt check/test/
# cd check
# ./configure CC=clang CFLAGS=$(SANITIZER_OPT)
# make -j
CHECKLIB=$(shell find ./check/src/.libs/ -name '*.o') #./check/src/.libs/libcheck.so
# extra errors in lprofWriteData fileWriter
COVERAGE =
endif

TESTFLAGS = `pkg-config --cflags check` $(COVERAGE) $(SANITIZER_OPT)
DEBUGFLAGS ?= $(TESTFLAGS) -g -Wall -Wextra -Wno-gnu-statement-expression -pedantic -Wno-empty-translation-unit -Wno-gnu-folding-constant
ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1
UBSAN_OPTIONS=print_stacktrace=1
#-Wno-reorder
OPTIMIZE = #-O3 -flto

# not used...
TARGET_ARCH =


CXXSTD = -std=c++14 
CSTD = -std=c11 # -D_POSIX_C_SOURCE=200809L


#CXX     = 
CC       = @ccache clang -fdiagnostics-color=always
rm       = rm -f
DEBUGENV = UBSAN_OPTIONS=print_stacktrace=1 ASAN_OPTIONS=detect_leaks=0 CK_FORK=no
DB       = gdb
PY3      = python3

# change these to set the proper directories where each files shoould be
SRCDIR  := src
OBJDIR  := obj
BINDIR  := bin
TESTDIR := test
LIBDIR  := lib

vpath $(SRCDIR) $(TESTDIR)

#BASEDIR = $(SRCDIR)/base
#FWDIRS  = $(sort $(dir $(wildcard $(FW)*/)))
SRCS = $(shell find $(SRCDIR)/ -name '*.c')
SRCS += $(shell find $(SRCDIR)/ -name '*.py')
TESTSRCS = $(shell find $(TESTDIR)/ -name '*.check')
OBJS := $(addsuffix .o,$(basename $(SRCS)))
OBJS := $(OBJS:$(SRCDIR)/%=$(OBJDIR)/%)
OBJS := $(filter-out $(OBJDIR)/main.o,$(OBJS))
TESTBINARIES = $(addsuffix .test,$(basename $(TESTSRCS)))
TESTBINARIES := $(TESTBINARIES:$(TESTDIR)/%=$(OBJDIR)/%)
TESTOBJS := $(addsuffix .check.o,$(basename $(TESTBINARIES)))
TESTRESULTS = $(addsuffix .log,$(basename $(TESTBINARIES)))
TESTDATA = $(addsuffix .profraw,$(basename $(TESTBINARIES)))


HEADERS = -iquote $(SRCDIR) -iquote $(OBJDIR) -Ijerryscript/jerry-core/include/ -Ijerryscript/jerry-ext/include/
#HEADERS = -I $(SRCDIR) -I $(OBJDIR)
CXXFLAGS = $(CXXSTD) $(HEADERS) $(DEBUGFLAGS) $(OPTIMIZE)
CFLAGS = $(CSTD) $(HEADERS) $(DEBUGFLAGS) $(OPTIMIZE)



NATIVEJS := $(LIBDIR)/libjerry-core.a $(LIBDIR)/libjerry-ext.a $(LIBDIR)/libjerry-port-default.a
LIBS := $(NATIVEJS)
#LDFLAGS = -L$(LIBDIR) -ljerry-core -ljerry-ext -ljerry-port-default #$(LIBS) # -static-libstdc++
LDFLAGS = $(LIBS) -lm # -static-libstdc++
LDTESTFLAGS = $(COVERAGE) $(CHECKLIB) $(LDFLAGS)

#OBJECTS  := 

EXECUTABLES := $(BINDIR)/core



.PHONY: clean debug test coverageclean coverage coverage-html all
all: $(EXECUTABLES) $(TESTBINARIES)
	
$(EXECUTABLES): | $(BINDIR)


$(BINDIR):
	mkdir -p $@
$(OBJDIR):
	mkdir -p $@
$(LIBDIR):
	mkdir -p $@



# Code generators:

$(OBJDIR)/%.check.c: $(TESTDIR)/%.check | $(OBJDIR)
	checkmk $< > $@

$(OBJDIR)/%.c: $(SRCDIR)/%.py | $(OBJDIR)
	$(PY3) $< c > $@
$(OBJDIR)/%.h: $(SRCDIR)/%.py | $(OBJDIR)
	$(PY3) $< h > $@

# fix clean compile by providing some deps manually:
$(OBJS): $(OBJDIR)/OmiConstants.h

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
$(OBJDIR)/%.o: $(OBJDIR)/%.c $(DEPDIR)/%.d | $(OBJDIR)
	@echo C $<
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPDIR)/%.d | $(OBJDIR)
	@echo C $<
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

%.d: ;

.PRECIOUS: %.d $(DEPDIR)/%.d $(OBJDIR)/%.check.o $(OBJDIR)/%.check.c $(OBJDIR)/%.c $(OBJDIR)/%.log $(OBJDIR)/%.o

#TODO: How to get dependencies for tests?
#$(OBJDIR)/%.test: $(OBJDIR)/%.check.o $(OBJDIR)/%.o $(LIBS)
$(OBJDIR)/%.test: $(OBJDIR)/%.check.o $(OBJS) $(LIBS)
	@echo "LINK $@"
	$(CC) -o $@ $(DEBUGFLAGS) $(OBJS) $< $(LDTESTFLAGS)
# save .map: -Xlinker -Map=% 

$(BINDIR)/core: $(OBJDIR)/main.o $(OBJS) $(LIBS)
	@echo "LINK $@"
	$(CC) -o $@ $(DEBUGFLAGS) $(OBJS) $< $(LDTESTFLAGS)



#$(OBJDIR)/%.o: $(SRCDIR)/%.c Makefile
#	@mkdir -p $(@D)
#	$(CC) $(MYCFLAGS) -c $< -o $@
#	@$(CC) $(MYCFLAGS) -MM $< -MF $(OBJDIR)/$*.d # dependencies
#	@sed -i '1s|^.*:|$@:|' $(OBJDIR)/$*.d # fix target path 

clean:
	$(rm) -rf $(OBJDIR)
	#$(rm) default.profraw
	$(rm) -rf $(LIBDIR)
	$(rm) -rf jerryscript/build
	for p in platforms/*; do cd $$p && make clean; done
	@echo "Cleanup complete!"

SHELL=/bin/bash -o pipefail

# Run tests, maybe print backtrace, run valgrind
$(OBJDIR)/%.log: $(OBJDIR)/%.test
	@echo -e "\nRUNNING TEST $<"
	@(LLVM_PROFILE_FILE="$(basename $<).profraw" ASAN_OPTIONS=$(ASAN_OPTIONS) ./$< | tee $@) || ($(DEBUGENV) $(DB) -q -ex run -ex bt -ex "kill inferiors 1" -ex quit $<; exit 1)

#@echo -e "\nRUNNING VALGRIND" | tee -a $@
#CK_FORK=no valgrind -q --leak-check=full $< >> $@

tags: $(SRCS)
	ctags -R src obj platforms

NEWESTSOURCE="${SRCDIR}/$(shell ls -t ${SRCDIR} | head -1)" 
SHOW_COVERAGE_LINES ?= 25
COVERAGE_OPTIONS ?= --instr-profile ./obj/default.profdata  --use-color --show-expansions --show-line-counts-or-regions
COVERAGE_FILTER ?= egrep --color=never '(\[0;41m|       \^0|\|      0\|)' -C 3 | sed 's/^--/┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅═┅/'
#COVERAGE_FILTER ?= awk -v ctx=3 -v lastf='' '{a[++i]=$$0 "\n";} /^\[0;/ {if (lastf!=$$0){lastf=$$0;f=1;}} /(\[0;41m|       \^0|\|      0\|)/{ if (NR-lastj>ctx){for (j=0;j<95;++j) printf "┅"; print "";if (f){print lastf "[0m";f=0;}} for(j=NR-ctx;j<NR+ctx;j++){printf a[j];a[j]="";lastj=j;}}'
#COVERAGE_FILTER ?= awk -v ctx=3 -v lastf='' '{a[++i]=$$0 "\n";} /^\[0;/ {if (lastf!=$$0){lastf=$$0;f=1;}} /(\[0;41m| {7}\^0|\| {6}0\|)/{ if (NR-lastj>ctx){for (j=0;j<95;++j) printf "┅"; print "";if (f){print lastf "[0m";f=0;}} for(j=NR-ctx;j<NR+ctx;j++){printf a[j];a[j]="";lastj=j;}}'

tests: $(TESTBINARIES)
test: $(TESTRESULTS) tags
	@echo
	@llvm-profdata merge -sparse $(TESTDATA) -o $(OBJDIR)/default.profdata
	@llvm-cov report --use-color -ignore-filename-regex='(yxml.*|.*.check.c)' --instr-profile=$(OBJDIR)/default.profdata $(TESTBINARIES) $(addprefix "--object=", $(TESTBINARIES)) | sed 's/-----------------------------------------//'
	@echo
	@echo "Some uncovered regions (search for 0 count lines if no red) in $(NEWESTSOURCE):"
	@llvm-cov show $(TESTBINARIES) $(NEWESTSOURCE) $(COVERAGE_OPTIONS) | $(COVERAGE_FILTER) | head -n $(SHOW_COVERAGE_LINES) || true

coverage:
	@llvm-cov show $(TESTBINARIES) -ignore-filename-regex='yxml*' $(COVERAGE_OPTIONS) | $(COVERAGE_FILTER)

coverage-html: ${COVERAGEDIR} $(TESTRESULTS)
	@llvm-cov show $(TESTBINARIES) --instr-profile ./obj/default.profdata --format=html --show-expansions --output-dir=${COVERAGEDIR} --ignore-filename-regex='yxml*'
	@echo Full coverage report: file://$(abspath ${COVERAGEDIR})/index.html

#@xdg-open file://$(abspath ${COVERAGEDIR})/index.html

coverageclean:
	@rm -f $(OBJDIR)/*.prof{raw,data} ${COVERAGEDIR}

ARGS?=$(EXECUTABLES)
debug:
	$(DEBUGENV) $(DB) $(ARGS)

.DELETE_ON_ERROR: %.o %.log

info:
	@echo $(TESTRESULTS)


# JERRY SCRIPT


$(NATIVEJS): | $(LIBDIR)
	@echo
	@echo MAKE JERRY SCRIPT
	@cd jerryscript; $(PY3) tools/build.py --clean --debug --lto=OFF --strip=OFF --profile $(abspath platforms/esp32s2/jerryscript.profile) --jerry-cmdline=OFF --external-context=ON --error-messages=ON --line-info=ON
	@cp jerryscript/build/lib/* $(LIBDIR)/
# parallel build fix (the first depends on the second)
$(word 1,$(NATIVEJS)): $(word 2,$(NATIVEJS))
$(word 2,$(NATIVEJS)): $(word 3,$(NATIVEJS))

CMD?=

esp32s2: $(OBJDIR)/OmiConstants.h $(OBJDIR)/OmiConstants.c
	@cd platforms/$@ && make $(CMD)

# DEPENDENSIES

-include $(OBJS:.o=.d)
-include $(TESTOBJS:.o=.d)
#include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))
