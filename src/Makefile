# find the OS
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

# if DEBUG env var is set, we compile with "debug" cflags
DEBUGFLAGS = -g -ggdb -O3 
ifeq ($(DEBUG), 1)
	DEBUGFLAGS = -g -ggdb -O0
endif

# Default CFLAGS
CFLAGS = -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-result -fPIC \
 	-D_GNU_SOURCE -std=gnu99 -I"$(shell pwd)" -DREDIS_MODULE_TARGET
CFLAGS += $(DEBUGFLAGS)

# Compile flags for linux / osx
ifeq ($(uname_S),Linux)
	SHOBJ_LDFLAGS ?= -shared -Bsymbolic -Bsymbolic-functions -ldl -lpthread
else
	CFLAGS += -mmacosx-version-min=10.12
	SHOBJ_LDFLAGS ?= -macosx_version_min 10.12 -exported_symbol _RedisModule_OnLoad -bundle -undefined dynamic_lookup -ldl -lpthread
endif
SHOBJ_LDFLAGS += $(LDFLAGS)

export CFLAGS

# Sources
SOURCEDIR=$(shell pwd -P)
CC_SOURCES = $(wildcard $(SOURCEDIR)/*.c)
CC_SOURCES += $(wildcard $(SOURCEDIR)/arithmetic/*.c)
CC_SOURCES += $(wildcard $(SOURCEDIR)/bulk_insert/*.c)
CC_SOURCES += $(wildcard $(SOURCEDIR)/execution_plan/*.c)
CC_SOURCES += $(wildcard $(SOURCEDIR)/execution_plan/ops/*.c)
CC_SOURCES += $(wildcard $(SOURCEDIR)/execution_plan/optimizations/*.c)
CC_SOURCES += $(wildcard $(SOURCEDIR)/filter_tree/*.c)
CC_SOURCES += $(wildcard $(SOURCEDIR)/graph/*.c)
CC_SOURCES += $(wildcard $(SOURCEDIR)/grouping/*.c)
CC_SOURCES += $(wildcard $(SOURCEDIR)/hexastore/*.c)
CC_SOURCES += $(wildcard $(SOURCEDIR)/parser/clauses/*.c)
CC_SOURCES += $(SOURCEDIR)/parser/ast.c
CC_SOURCES += $(SOURCEDIR)/parser/ast_common.c
CC_SOURCES += $(SOURCEDIR)/parser/ast_arithmetic_expression.c
CC_SOURCES += $(SOURCEDIR)/parser/lex.yy.c
CC_SOURCES += $(SOURCEDIR)/parser/grammar.c
CC_SOURCES += $(wildcard $(SOURCEDIR)/resultset/*.c)
CC_SOURCES += $(wildcard $(SOURCEDIR)/stores/*.c)
CC_SOURCES += $(wildcard $(SOURCEDIR)/util/*.c)
CC_SOURCES += $(wildcard $(SOURCEDIR)/util/datablock/*.c)
CC_SOURCES += $(wildcard $(SOURCEDIR)/util/thpool/*.c)
CC_SOURCES += $(wildcard $(SOURCEDIR)/index/*.c)

# Convert all sources to .o files
CC_OBJECTS = $(patsubst %.c, %.o, $(CC_SOURCES) )
export CC_OBJECTS

# .d files for each c file. These make sure that changing a header file
# will also change the dependent .c files of it
CC_DEPS = $(patsubst %.c, %.d, $(CC_SOURCES) )

# Library dependencies
LIBRMUTIL=rmutil/librmutil.a
LIBTRIEMAP=util/triemap/libtriemap.a
GRAPHBLAS=../deps/GraphBLAS/build/libgraphblas.a

# Compilation deps for the module
LIBS=$(LIBTRIEMAP) $(LIBRMUTIL) $(GRAPHBLAS)
MODULE=$(CC_OBJECTS) $(LIBS)

%.c: %.y

# Compile C file while generating a .d file for it
%.o: %.c
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -MMD -MF $(@:.o=.d)

all: redisgraph.so

# Include all dependency files for C files
-include $(CC_DEPS)

# Library compile rules
$(LIBRMUTIL):
	$(MAKE) -C rmutil
.PHONY: $(LIBRMUTIL)

$(LIBTRIEMAP):
	$(MAKE) -C util/triemap
.PHONY: $(LIBTRIEMAP)

$(GRAPHBLAS):
	$(MAKE) CMAKE_OPTIONS="-DCMAKE_CXX_FLAGS=-std=c++11" library -C ../deps/GraphBLAS

.PHONY: $(GRAPHBLAS)

# Compile query parse. 
# This is not included in the usual make target!
parser:
	$(MAKE) -C $@
.PHONY: parser

# Build the module...
redisgraph.so: $(MODULE)
	$(LD) -o $@ $(MODULE) $(SHOBJ_LDFLAGS) -lc -lm

clean:
	find . -name '*.[oad]' -type f -delete
	$(MAKE) -C ../tests clean

package: redisgraph.so
	mkdir -p ../build
	ramp pack -m "`pwd`/../ramp_manifest.yml" -v -o "../build/redisgraph.{os}-{architecture}.latest.zip" "`pwd`/redisgraph.so"

test: $(MODULE) redisgraph.so
	# unit tests
	$(MAKE) -C ../tests test
