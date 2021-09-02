all: cache.c
	gcc -Wall -Werror -fsanitize=address -o cache cache.c -std=c99 -lm -g

clean:
	rm -f cache
