SWITCH= -DTOOLSTEST
OBJS= JSONError.o JSONHelper.o JSONBuilder.o JSONOutput.o JSONParser.o
FLAGS=-Iinclude/ --std=c99 -ggdb -Wall
		
staticlib: $(OBJS)
		ar -cr libjsontools.a $(OBJS)
		mv libjsontools.a ../lib

all: test.c staticlib
		gcc $(FLAGS) test.c -L ../lib -ljsontools -o JSONTest
		mv JSONTest ../bin

JSONParser.o: JSONParser.c
		gcc -c $(FLAGS) JSONParser.c -o JSONParser.o
		
JSONBuilder.o: JSONBuilder.c
		gcc -c $(FLAGS) JSONBuilder.c -o JSONBuilder.o
		
JSONOutput.o: JSONOutput.c
		gcc -c $(FLAGS) JSONOutput.c -o JSONOutput.o
		
JSONError.o: JSONError.c
		gcc -c $(FLAGS) JSONError.c -o JSONError.o
		
JSONHelper.o: JSONHelper.c
		gcc -c $(FLAGS) JSONHelper.c -o JSONHelper.o

clean:
		rm -rf *.o
		
spotless: 
		-rm -rf *.o
		-rm ../bin/JSONTest
		-rm ../lib/*.a

.PHONY: clean spotless

