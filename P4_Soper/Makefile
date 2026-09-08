CC = gcc -pedantic -lrt -g
CFLAGS = -Wall 
EXE  = monitor
EXE2 = miner
SRCS = monitor.c pow.c miner.c shm_miner.c buffer.c
OBJS =  $(SRCS:.c=.o)

all : $(EXE) $(EXE2)

$(EXE) : monitor.o pow.o buffer.o
	$(CC) $(CFLAGS) -o $@  $^ -lpthread -lrt

$(EXE2) : shm_miner.o miner.o pow.o 
	$(CC) $(CFLAGS) -o $@  $^ -lpthread -lrt

%.o : %.c
	$(CC) $(CFLAGS) -c $<

.PHONY : clean
clean :
	rm -f *.o core $(EXE) $(EXE2)

