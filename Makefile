SWITCH= -DTOOLSTEST
OBJS= jsonerror.o jsonhelper.o jsonbuilder.o jsonoutput.o jsonparser.o
FLAGS= --std=gnu99 -Wall
#FLAGS += -ggdb
FLAGS += -O3
		
all: jsontools.c version.h staticlib
		gcc $(FLAGS) jsontools.c -L . -ljsontools -o jsontools

staticlib: $(OBJS)
		ar -cr libjsontools.a $(OBJS)

jsonparser.o: jsonparser.c
		gcc -c $(FLAGS) jsonparser.c -o jsonparser.o
		
jsonbuilder.o: jsonbuilder.c
		gcc -c $(FLAGS) jsonbuilder.c -o jsonbuilder.o
		
jsonoutput.o: jsonoutput.c
		gcc -c $(FLAGS) jsonoutput.c -o jsonoutput.o
		
jsonerror.o: jsonerror.c
		gcc -c $(FLAGS) jsonerror.c -o jsonerror.o
		
jsonhelper.o: jsonhelper.c
		gcc -c $(FLAGS) jsonhelper.c -o jsonhelper.o

version.h: version.txt
		echo "#define VERSION \"$(shell cat version.txt)\"" > version.h
		echo "#define REVISION \"$(shell git log -1 | grep commit | cut -d " " -f 2)\"" >> version.h

install:
		mkdir -p /usr/local/lib
		cp libjsontools.a /usr/local/lib
		mkdir -p /usr/local/include/jsonTools
		cp include/*.h /usr/local/include/jsonTools
		cp jsontools /usr/local/bin

clean:
		rm -rf *.o
		rm -rf libjsontools.a
		rm -rf jsontools
		rm -rf version.h
		
.PHONY: clean install

