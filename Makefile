CFLAGS = -O0 -Wall -ggdb -std=c99

all: clean out/spectre_v1 out/spectre_v2

out/spectre_v1: ./spectre/spectre_v1.c
	$(CC) $(CFLAGS) -o $@ $^

out/spectre_v2: ./spectre/spectre_v2.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f out/**

out/:
	mkdir -p out
