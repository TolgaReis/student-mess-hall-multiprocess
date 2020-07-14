program: program.c program-utils.c
	gcc -o program program.c program-utils.c -pthread -lrt
clean:
	rm -f program 