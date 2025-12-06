.DEFAULT_GOAL := all

all: build

build:
	cc main.c -Wall -W -O2 -o cq

clean:
	rm -f cq

test: clean build
	./cq test_data.csv