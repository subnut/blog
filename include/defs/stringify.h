#ifndef STRINGIFY

/*
 * Can be used to get the value of pre-processor macro in string form
 */

#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY(x) #x

#endif
