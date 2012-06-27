SWITCH= -DTOOLSTEST
OBJS= JSONError.o JSONHelper.o JSONBuilder.o JSONOutput.o JSONParser.o
FLAGS=-Iinclude/ --std=c99 -Wall
#FLAGS += -ggdb
FLAGS += -O3
		
all: jsontools.c version.h staticlib
		gcc $(FLAGS) jsontools.c -L . -ljsontools -o jsontools

staticlib: $(OBJS)
		ar -cr libjsontools.a $(OBJS)

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

version.h: version.txt
		echo "#define VERSION \"$(shell cat version.txt)\"" > version.h
		echo "#define REVISION \"$(shell git log -1 | grep commit | cut -d " " -f 2)\"" >> version.h

install:
		cp libjsontools.a /usr/local/lib
		mkdir -p /usr/local/include/JSONTools
		cp include/*.h /usr/local/include/JSONTools
		cp jsontools /usr/local/bin

clean:
		rm -rf *.o
		rm -rf libjsontools.a
		rm -rf jsontools
		rm -rf version.h
		
.PHONY: clean install

