# Target & directories
TEST_TARGET = llh_test
DIR_TEST_SOURCE = test
DIR_TEST_TARGET = bin
DIR_OBJECT = obj

# Source files
TEST_SOURCES = llh.c
TEST_OBJECTS = $(TEST_SOURCES:%.c=$(DIR_OBJECT)/%.o)
DEPENDENCIES = $(TEST_OBJECTS:%.o=%.d)

# Compiler options
CC = clang
CFLAGS += -std=gnu11 -g -O -Weverything -pedantic
SEARCH_INCLUDES = -Iinclude

# Automatically create intermediate directories
dir_check = @mkdir -p $(@D)

.PHONY: all
all: $(DIR_TEST_TARGET)/$(TEST_TARGET)

.PHONY: clean
clean:
	rm -f $(DIR_TEST_TARGET)/$(TEST_TARGET)
	rm -f $(TEST_OBJECTS)
	rm -f $(DEPENDENCIES)

$(DIR_TEST_TARGET)/$(TEST_TARGET): $(TEST_OBJECTS)
	$(dir_check)
	$(CC) $(LDFLAGS) -o $(DIR_TEST_TARGET)/$(TEST_TARGET) $(TEST_OBJECTS) $(SEARCH_LIBRARIES) $(LIBRARIES)

-include $(DEPENDENCIES)

$(DIR_OBJECT)/%.o: $(DIR_TEST_SOURCE)/%.c
	$(dir_check)
	$(CC) $(CFLAGS) -MMD -o $@ -c $(SEARCH_INCLUDES) -c $<
