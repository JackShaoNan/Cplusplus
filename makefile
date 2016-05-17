test2:main.o my_print.o
	gcc -m32 -o test2 main.o my_print.o
main.o:main.c
	gcc -m32 -c main.c
my_print.o:my_print.asm
	nasm -f elf my_print.asm -o my_print.o
clean:
	rm test2 main.o my_print.o
