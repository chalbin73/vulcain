CXX=clang++

all: lib/libvma.a

lib/libvma.a: obj/vk_mem_alloc.o
	ar rcs lib/libvma.a obj/vk_mem_alloc.o

obj/vk_mem_alloc.o: include/vk_mem_alloc.h
	$(CXX) -std=c++17 -c src/lib.cpp -Iinclude -o obj/vk_mem_alloc.o -fPIC -lvulkan

clean:
	rm obj/vk_mem_alloc.o
	rm lib/libvma.a