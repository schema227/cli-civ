CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c17 -g -Iinclude
DEPFLAGS := -MMD -MP
TARGET := civ_console
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
DEP := $(OBJ:.o=.d)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(DEP) $(TARGET)

-include $(DEP)
