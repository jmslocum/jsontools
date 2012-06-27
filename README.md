# JSONTools
JSONTools is both a standalone command line tool for verifying, building, and printing json cleanly, and 
a library that can be linked against. It was developed from the stand point of having a DOM style json 
parser and message builder. When parsing a json message, the entire message is scanned and a data structure 
is built and held in memory. This data structure can then be "queried" for information you are interested in.

## Command line tool
The jsontools command line tools is a quick interface and demo program for how to use the library. The
tool can be used to print a json file "pretty" or to verify that the message is valid. 

```
jsontools usage: 
    jsontools [options] file1 file2 ... fileN
options:
    -h  --help    Print this messsage
    -v  --version Print the version number
    -r  --verify  only a 0 or 1 return value
                  0 = good json, positive number = bad

```

## Library
The JSONtools library is an opensource static library that can be linked against other C or C++ programs.
For a quick example of how to use the library, check out jsontools.c. More instructions will be comming.
but you basicly have to include the JSONTools.h header file (which will include all of the others) and
link against the library.

## Compiling and Installing
The library was written to use only the C standard library so it should compile on any system. However, the
jsontools program was written with unix libraries, so it wont compile on a non Unix system. As I test the
library on more systems I will add them below, but for now it SHOULD compile and install without issue on
Linux, BSD, OSX, and Cygwin. 

```
make
sudo make install
make clean
```

libjsontools.a installs to /usr/local/lib

the headers are installed to /usr/local/include/JSONTools

jsontools installs to /usr/local/bin

## Reporting issues
When reporting any issues, please include the version number and revision number (you will notice the 
revision looks a lot like a git commit hash). That way I can target the exact code base the problem 
occurred on. Also if possible include the exact json message that you were parsing when the issue 
occurred. 

Alwasy make sure to check for new versions before reporing an error, as I may have already fixed
the issue in a new version. 

## License
Copyright 2012
All rights reserved 
Programs written by James Slocum

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
documentation files (the "Software"), to deal in the Software without restriction, including without 
limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.
