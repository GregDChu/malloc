LIB = libstd/
LIB32 = lib/
LIB64 = lib64/

main: asn01.o libmalloc.so libpath
	gcc  -o main asn01.o -L$(LIB) -lmalloc

asn01.o: asn01.c
	gcc -g -w -c asn01.c -o asn01.o

libmalloc.so: malloc.c malloc.h
	rm -r -f $(LIB)
	mkdir $(LIB)
	gcc -g -w -fPIC -c -o $(LIB)malloc.o malloc.c
	gcc -g -w -fPIC -shared -o $(LIB)libmalloc.so $(LIB)malloc.o
	ar r $(LIB)libmalloc.a $(LIB)malloc.o

intel-all: malloc64.o malloc32.o
	gcc -g -w -fPIC -m32 -shared -o $(LIB32)libmalloc.so $(LIB32)malloc32.o
	gcc -g -w -fPIC -m64 -shared -o $(LIB64)libmalloc.so $(LIB64)malloc64.o
	ar r $(LIB32)libmalloc.a $(LIB32)malloc32.o
	ar r $(LIB64)libmalloc.a $(LIB64)malloc64.o
	rm -f *.o */*.o

malloc64.o: malloc.c malloc.h
	rm -r -f $(LIB64)
	mkdir $(LIB64)
	gcc -g -w -fPIC -m64 -c -o $(LIB64)malloc64.o malloc.c 

malloc32.o: malloc.c malloc.h
	rm -r -f $(LIB32)
	mkdir $(LIB32)
	gcc -g -w -fPIC -m32 -c -o $(LIB32)malloc32.o malloc.c

libpath:
	export LD_LIBRARY_PATH=./$(LIB):$$LD_LIBRARY_PATH

clean:
	rm -f *.o */*.o

clear: clean
	rm -f *.so main *.a
	rm -r -f */
	