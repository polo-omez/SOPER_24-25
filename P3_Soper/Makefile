CC = gcc -pedantic -lrt -g
CFLAGS = -Wall 
EXE  = monitor
EXE2 = miner
SRCS = monitor.c pow.c miner.c buffer.c 
OBJS =  $(SRCS:.c=.o)

all : $(EXE) $(EXE2)

.PHONY : clean
clean :
	rm -f *.o core $(EXE) $(EXE2)

$(EXE) : monitor.o pow.o buffer.o
	$(CC) $(CFLAGS) -o $@  $^ -lpthread

$(EXE2) : miner.o pow.o
	$(CC) $(CFLAGS) -o $@  $^ -lpthread

%.o : %.c 
	$(CC) $(CFLAGS) -c $<

