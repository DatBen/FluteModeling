all: main

build/utils.o: src/utils.c include/utils.h
	gcc -o build/utils.o -c src/utils.c -O3 

build/main.o: src/main.c include/flute.h
	gcc -o build/main.o -c src/main.c -O3 

main: build/utils.o build/main.o 
	gcc -o main build/main.o build/utils.o -O3 

make clean:
	rm -f build/*.o ./main vti/*.vti 