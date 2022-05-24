all: main


main: src/utils.c src/main.c 
	gcc -o main src/main.c src/utils.c -lOpenCL 

make clean:
	rm -f build/*.o ./main vti/*.vti 