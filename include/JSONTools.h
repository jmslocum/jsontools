#ifndef _JSON_TOOLS_H
#define _JSON_TOOLS_H

#ifndef _ISOC99_SOURCE
#define _ISOC99_SOURCE
#endif

/**
 * \mainpage Welcome to the JSONTools library documentation page
 * \section about_sect About JSONTools
 * JSONTools was created as an alternative JSON parsing library for C. 
 * Most other JSON libraries are SAX style parsers that utilize 
 * different callback methods to interface with a users program. 
 * Although there are situations in which that proves a good design, 
 * It is not always the best option. JSONTools is different in that 
 * it is a DOM style parser. The message being parsed is read into 
 * memory and stored in a document object. That document object can 
 * then be passed around and queried against. This allows you to easily
 * extract only the data that your want from your message, and not 
 * need to respond to each callback and waste time. The library also 
 * has a builder option to build a document object using predefined 
 * functions. That document object can then be passed to the output
 * module and converted into a text JSON message to be stored on the 
 * hard drive, sent via a network connection, or displayed to the user.
 * 
 * \section install_sect Installation
 * To build and use the JSONTools library, run the make command. This 
 * will compile libjsontools.a and place it in the lib directory. Then
 * you can use the library by linking your project agienst it. 
 * 
 */

/**
 * This should be the file included when using the libjsontools library.
 * The other headers provide more details about the implementation and
 * usage, but have some cross dependencies. It is best not to include
 * them individually. 
 */

#include "JSONCommon.h"
#include "JSONParser.h"
#include "JSONBuilder.h"
#include "JSONOutput.h"
#include "JSONError.h"
#include "JSONHelper.h"

#endif

