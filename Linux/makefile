CC=g++
#CFLAGS=-std=c++11 -Wall -g3 -DJUAN_DEBUG -DUMP_STIM
CFLAGS=-std=c++11 -Wall -g3 -DDUMP_STIM
LFLAGS=-Wall
DEPS=frame.h matrix.h residual.h golomb.h util.h global_variable.h
OBJS=frame.o matrix.o residual.o golomb.o util.o
OUT=encode 

all: $(OUT) 

%.o : %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

encode: encode.o $(OBJS)
	$(CC) $(LFLAGS) -o $@ $^	
	
clean:
	rm $(OBJS) encode.o decode.o $(OUT)
