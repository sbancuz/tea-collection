CC := gcc
CFLAGS = -O0 -Wall -ggdb -std=c99
SRC_DIRS := spectre meltdown lvi experiments


SRC_FILES := $(wildcard $(addsuffix /*.c, $(SRC_DIRS)))
TARGETS := $(patsubst %, out/%, $(SRC_FILES))

all: $(TARGETS)

out/%: % 
	@mkdir -p $(@D)
	-$(CC) $(CFLAGS) -o $@ $<

.PHONY: all clean .FORCE

clean:
	rm -rf out/*
