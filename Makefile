\CFLAGS=-Wno-unused-function -Wall -Wextra -Werror -O2 
TARGETS= lab1kdaN3248 libkdaN3248.so

.PHONY: all clean

all: $(TARGETS)

clean:
	rm -rf *.o $(TARGETS)

lab1kdaN3248: lab1kdaN3248.c plugin_api.h
	gcc $(CFLAGS) -g -o lab1kdaN3248 lab1kdaN3248.c -ldl

libkdaN3248.so: libkdaN3248.c plugin_api.h
	gcc $(CFLAGS) -g -shared -fPIC -o libkdaN3248.so libkdaN3248.c -ldl -lm

