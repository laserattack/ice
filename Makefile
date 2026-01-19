.PHONY: all clean leaks cloc

CC       = cc
CFLAGS   = -Wall -Wextra -std=c99

VALGRIND = valgrind
VFLAGS   = --leak-check=full --show-leak-kinds=all --track-origins=yes

CLOC     = cloc
CLFLAGS  = --exclude-dir=thirdparty

TARGET   = ice
SOURCES  = ice.c linelist.c common.c
OBJECTS  = $(SOURCES:.c=.o)
DEPS     = $(SOURCES:.c=.d)

all: $(TARGET)

leaks: all
	$(VALGRIND) $(VFLAGS) ./$(TARGET)

cloc:
	$(CLOC) . $(CLFLAGS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MF $*.d -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPS)
