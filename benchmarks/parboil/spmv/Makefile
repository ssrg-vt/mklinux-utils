# (c) 2010 The Board of Trustees of the University of Illinois

BIN := spmv

SRC := $(shell ls *.c)
HDR := $(shell ls *.h)

OMP_LIB := -lgomp

$(BIN): $(SRC) $(HDR)
	gcc -fopenmp -O3 -o $(BIN) -I./ $(SRC) -lm $(OMP_LIB)
	
spmv-pomp:
	gcc -c main.c mmio.c parboil.c file.c convert_dataset.c $(CC_FLAGS) -fopenmp -DBOMP $(OPTFLAGS) $(DBGFLAGS) -I./
	gcc -o spmv-pomp main.o mmio.o parboil.o file.o convert_dataset.o -lm ../../lib/libpomp.a $(OPTFLAGS) $(DBGFLAGS) 

clean:
	rm -rf $(BIN) Dubcova3.mtx.out

.PHONY: clean
