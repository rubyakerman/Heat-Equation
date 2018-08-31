CC = gcc
FLAGS = -c -Wall -Wvla -std=c99
CODEFILES = ex3.tar reader.c calculator.c Makefile
ARGS = input.txt

# Creating an executable-file its name is ex3
ex3: reader.o calculator.o heat_eqn.o
	$(CC) reader.o calculator.o heat_eqn.o -o ex3

# Calling to ex3 target & than running the program (which ex3 target buit for us)
# with file named input.txt as an argument
all: ex3
	./ex3 input.txt

# Object files: 
reader.o: reader.c calculator.h
	$(CC) $(FLAGS) reader.c -o reader.o

calculator.o: calculator.c calculator.h
	$(CC) $(FLAGS) calculator.c -o calculator.o

heat_eqn.o: heat_eqn.c heat_eqn.h
	$(CC) $(FLAGS) heat_eqn.c -o heat_eqn.o
 
# other targets:
run: 
	make clean
	make
	ex3 $(ARGS)

tar: 
	tar cvf $(CODEFILES)

clean: 
	rm -f *.o ex3