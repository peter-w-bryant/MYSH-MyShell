CC=gcc

mysh: mysh.c
	@$(CC) -o mysh -Wall -Werror -g mysh.c

clean:
	rm -f mysh
