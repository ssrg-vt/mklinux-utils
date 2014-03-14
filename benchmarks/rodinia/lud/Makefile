BIN := lud

SRC := $(shell ls *.c)
HDR := $(shell ls *.h)

OMP_LIB := -lgomp

$(BIN): $(HDR) $(SRC)
	gcc -O3 -fopenmp -o $(BIN) $(SRC) -lm $(OMP_LIB)

lud-gomp:
	
lud-bomb:
	
lud-pomp:
	$(CC) -c lud.c lud_omp.c common.c $(CC_FLAGS) -fopenmp -DBOMP $(OPTFLAGS) $(DBGFLAGS)
	gcc -o lud-pomp lud.o lud_omp.o common.o -lm ../../lib/libpomp.a $(OPTFLAGS) $(DBGFLAGS) 

clean:
	rm -rf $(BIN)

.PHONY: clean
