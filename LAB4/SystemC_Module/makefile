ECHO		:=echo -e
CC			:=gcc
CXX			:=g++
LD			:=g++
MKDIR_P		:=mkdir -p
INCDIR		:=$(patsubst %,-I%, $(shell find include -type d))
SRCDIR		:=source
SYSTEMC_DIR	:=$(shell echo ~)/workspace/SystemC
DEPDIR		:=.dep
EXTSRC		:=
BUILDDIR	:=build
LIBDIR		:=$(BUILDDIR)/extlib
CEXT		:=c
CXXEXT		:=cpp
OBJEXT		:=o

INC			:= $(INCDIR) -I$(SYSTEMC_DIR)/include
LIB			:= -L $(SYSTEMC_DIR)/lib-linux64
DEFINE		:= 
#DEFINE		+= -D__DEBUG__

#CFLAGS		:=-c -Wall -o3 $(INC) $(DEFINE)
CFLAGS		:=-c -Wall -g -O0 $(INC) $(DEFINE)
LDFLAGS		:=$(LIB) -pthread -lsystemc -lm
LDFLAGS_END	:=-lrt
EXTSOURCES	:=$(abspath $(EXTSRC))
EXTOBJECTS	:=$(addprefix $(LIBDIR),$(EXTSOURCES:.$(CEXT)=.$(OBJEXT)))
SOURCES		:=$(patsubst %.$(CEXT), %.$(OBJEXT), $(shell find $(SRCDIR) -name *.$(CEXT))) $(patsubst %.$(CXXEXT), %.$(OBJEXT), $(shell find $(SRCDIR) -name *.$(CXXEXT)))
OBJECTS		:=$(EXTOBJECTS) $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(CEXT)=.$(OBJEXT)))

EXECUTABLE	:=qsysbridge

.PONY: all
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	@$(ECHO) '[LD]\t$@'
	@$(LD) $(LDFLAGS) $^ -o $@ $(LDFLAGS_END)

$(BUILDDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(CEXT)
	@$(MKDIR_P) $(dir $@)
	@$(ECHO) '[CC]\t$<'
	@$(CC) $(CFLAGS) $< -o $@

$(BUILDDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(CXXEXT)
	@$(MKDIR_P) $(dir $@)
	@$(ECHO) '[CXX]\t$<'
	@$(CXX) $(CFLAGS) $< -o $@

$(EXTOBJECTS): $(LIBDIR)%.$(OBJEXT): %.$(CEXT)
	@$(MKDIR_P) $(dir $@)
	@$(ECHO) '[CC]\t$(shell realpath --relative-to=`pwd` $<)'
	@$(CC) $(CFLAGS) $< -o $@

$(EXTOBJECTS): $(LIBDIR)%.$(OBJEXT): %.$(CXXEXT)
	@$(MKDIR_P) $(dir $@)
	@$(ECHO) '[CXX]\t$(shell realpath --relative-to=`pwd` $<)'
	@$(CXX) $(CFLAGS) $< -o $@

clean:
	@rm -f $(EXECUTABLE)
	@rm -rf $(BUILDDIR)
	@rm -rf $(DEPDIR)
	@$(ECHO) '[DEL]\t$(EXECUTABLE)'
	@$(ECHO) '[RMDIR]\t$(BUILDDIR)'
