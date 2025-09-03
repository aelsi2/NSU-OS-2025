SRC_DIRS = ./src
INC_DIRS = $(SRC_DIRS)
BUILD_DIR = ./build
TARGET_EXEC = $(BUILD_DIR)/program.out

FIND_SRCS = find $(SRC_DIRS) -name '*.c'
SRCS = $(FIND_SRCS:sh)
OBJS = $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:%=%.d)

# All GCC-generated dependency files concatenated together
DEPS_CAT = $(BUILD_DIR)/deps.d
# Needed for LSP
COMPILE_FLAGS_FILE = compile_flags.txt

CC = gcc
CFLAGS = $(INC_DIRS:%=-I%) -MMD -MP -Wall

$(TARGET_EXEC): $(OBJS) $(COMPILE_FLAGS_FILE) $(DEPS_CAT)
	$(LINK.c) $(OBJS) -o $@

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(@D)
	$(COMPILE.c) -MF $@.d $< -o $@

$(COMPILE_FLAGS_FILE):
	echo "$(CFLAGS:%=%\n)" > $@

$(DEPS_CAT): $(DEPS)
	mkdir -p $(@D)
	cat $(DEPS) > $@

$(BUILD_DIR)/%.o.d: $(BUILD_DIR)/%.o

clean:
	rm -f $(COMPILE_FLAGS_FILE)
	rm -rf $(BUILD_DIR)

MAYBE_DEPS_CAT = if test -f $(DEPS_CAT); then echo $(DEPS_CAT); else echo /dev/null; fi
include $(MAYBE_DEPS_CAT:sh)
