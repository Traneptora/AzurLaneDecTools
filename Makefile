CC=gcc -m32 -march=i386 -msse4.1
CXX=g++ -m32 -march=i386 -msse4.1
RANLIB=ranlib
LUAJIT_A=libluajit.a

all: bcdec

libluajit.a:
	$(MAKE) -C src $(LUAJIT_A)
	cp src/$(LUAJIT_A) ./
	$(RANLIB) $(LUAJIT_A)

bcdec: libluajit.a compat.o main.cpp ByteCodeDec.cpp
	$(CXX) -std=c++17 -o bcdec main.cpp ByteCodeDec.cpp compat.o -Isrc -L. -lluajit -ldl -latomic

%.o: %.c
	$(CC) -c -o $@ $<

clean:
	rm -f -- bcdec libluajit.a compat.o
	$(MAKE) -C src clean

.PHONY: all clean
