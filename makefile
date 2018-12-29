CC = gcc
FLAGS = "-Wall"

%_test: test/%_test.c
	$(CC) $(FLAGS) $< -o testbin/$@ > log/compilation_$@.log 2> log/compilation_$@.err

