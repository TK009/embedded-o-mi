
TARGET_ARCH =
TESTFLAGS = `pkg-config --cflags check` -fprofile-instr-generate -fcoverage-mapping
DEBUGFLAGS = $(TESTFLAGS) -g -Wall -Wextra -Wno-gnu-statement-expression -pedantic -Wno-empty-translation-unit
#-Wno-reorder
OPTIMIZE = #-O3 -flto


CXXSTD = -std=c++14 
CSTD = -std=c11 


#CXX    = g++
CC     = clang
rm     = rm -f

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
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPDIR)/%.d | $(OBJDIR)
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

%.d: ;

.PRECIOUS: %.d $(DEPDIR)/%.d $(OBJDIR)/%.check.o $(OBJDIR)/%.check.c $(OBJDIR)/%.log $(OBJDIR)/%.o

#$(TESTBINARIES)
$(OBJDIR)/%.test: $(OBJDIR)/%.check.o $(OBJS)
	@echo
	@echo "LINKING $@!"
	$(CC) -o $@ $(LDTESTFLAGS) $(DEBUGFLAGS) $^
	@echo "LINKING $@ complete!"




#$(OBJDIR)/%.o: $(SRCDIR)/%.c Makefile
#	@mkdir -p $(@D)
#	$(CC) $(MYCFLAGS) -c $< -o $@
#	@$(CC) $(MYCFLAGS) -MM $< -MF $(OBJDIR)/$*.d # dependencies
#	@sed -i '1s|^.*:|$@:|' $(OBJDIR)/$*.d # fix target path 

.PHONEY: clean debug test coverageClean
clean:
	$(rm) -rf $(OBJDIR)
	@echo "Cleanup complete!"

SHELL=/bin/bash -o pipefail

#@rm -f $(BIN)/*.gcd{a,o}
$(OBJDIR)/%.log: $(OBJDIR)/%.test
	@echo -e "\nRUNNING TEST $<"
	@LLVM_PROFILE_FILE="$(basename $<).profraw" ./$< | tee $@
	@echo -e "\nRUNNING VALGRIND" | tee -a $@
	valgrind --leak-check=full $< >> $@

	valgrind --leak-check=full $< >> $@


test: $(TESTRESULTS)
	@echo
	@llvm-profdata merge -sparse $(TESTDATA) -o $(OBJDIR)/default.profdata
	@llvm-cov report -instr-profile=$(OBJDIR)/default.profdata $(OBJDIR)/odf.test

coverageclean:
	@rm -f $(OBJDIR)/*.prof{raw,data}

.DELETE_ON_ERROR: %.o %.log

-include $(OBJS:.o=.d)
#include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))
