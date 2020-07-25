CC=gcc -m32 -march=i386 -msse4.1
CXX=g++ -std=c++17 -m32 -march=i386 -msse4.1
RANLIB=ranlib
LUAJIT_A=libluajit.a
INCLUDES=-Isrc

all: bcdec

libluajit.a:
	$(MAKE) -C src $(LUAJIT_A)
	cp src/$(LUAJIT_A) ./
	$(RANLIB) $(LUAJIT_A)

bcdec: libluajit.a compat.o bcdec.o ByteCodeDec.o
	$(CXX) -std=c++17 -o bcdec bcdec.o ByteCodeDec.o compat.o  -L. -lluajit -ldl -latomic

%.o: %.c
	$(CC) -c -o $@ $< $(INCLUDES)
%.o: %.cpp
	$(CXX) -c -o $@ $< $(INCLUDES)

clean:
	rm -f -- bcdec libluajit.a compat.o
	$(MAKE) -C src clean

.PHONY: all clean
