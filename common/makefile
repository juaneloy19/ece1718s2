CC=g++
CFLAGS=-std=c++11 -Wall -g3
LFLAGS=-Wall
DEPS=frame.h matrix.h residual.h golomb.h util.h
OBJS=frame.o matrix.o residual.o golomb.o util.o
OUT=encode decode

all: $(OUT) 

%.o : %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

decode: decode.o $(OBJS)
	$(CC) $(LFLAGS) -o $@ $^
	
encode: encode.o $(OBJS)
	$(CC) $(LFLAGS) -o $@ $^	
	
clean:
	rm $(OBJS) encode.o decode.o $(OUT)
