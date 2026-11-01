CC      ?= gcc
CFLAGS  = -std=c99 -Wall -Wextra -O2
SRC     = $(wildcard src/*.c)
TARGET  = dashboard

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe trip_log.txt

.PHONY: all run clean
