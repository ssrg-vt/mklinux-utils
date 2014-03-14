# C compiler
CC = g++
CC_FLAGS = -g -fopenmp -O3

OMP_LIB := -lgomp

all: bfs-gomp bfs-bomp bfs-pomp

bfs-gomp: 
	gcc -c wtime.c c_timers.c -DPOSIX $(OPTFLAGS) $(DBGFLAGS)
	$(CC) -o bfs-gomp $(CC_FLAGS) bfs.cpp c_timers.o wtime.o $(OMP_LIB)

bfs-bomp:
	gcc -c wtime.c c_timers.c -DPOSIX $(OPTFLAGS) $(DBGFLAGS)
	$(CC) -o bfs-bomp $(CC_FLAGS) bfs.cpp c_timers.o wtime.o -DBOMP -fopenmp ../../lib/libbomp.a -lpthread -lnuma $(OPTFLAGS) $(DBGFLAGS) 

bfs-pomp:
	gcc -c wtime.c c_timers.c -DPOSIX $(OPTFLAGS) $(DBGFLAGS)
	$(CC) -c bfs.cpp $(CC_FLAGS) -DBOMP $(OPTFLAGS) $(DBGFLAGS)
	gcc -o bfs-pomp bfs.o c_timers.o wtime.o -lstdc++ ../../lib/libpomp.a $(OPTFLAGS) $(DBGFLAGS) 

clean:
	rm -f bfs-gomp bfs-bomp bfs-pomp result.txt
