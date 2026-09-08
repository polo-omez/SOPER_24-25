CC = gcc -ansi -pedantic
CFLAGS = -Wall
EXE  = voting
SRCS = handler.c candidato.c votante.c  main.c utils.c
OBJS =  $(SRCS:.c=.o)

all : $(EXE)

.PHONY : clean
clean :
	rm -f *.o core $(EXE)

$(EXE) : $(OBJS)
	$(CC) $(CFLAGS) -o $@  $^ -lpthread

%.o : %.c 
	$(CC) $(CFLAGS) -c $<

