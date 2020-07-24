OBJ=CPU_cache

all:
	gcc $(OBJ).c -o $(OBJ) -lm
clean:
	rm $(OBJ)
