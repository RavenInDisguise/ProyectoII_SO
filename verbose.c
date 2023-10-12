#include "verbose.h"
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

bool Verbose = false;
bool StrongVerbose = false;

void setVerbose(bool setting) {
    Verbose = setting;
}

void setStrongVerbose(bool setting) {
    StrongVerbose = setting;
}

int verbose(const char * restrict format, ...) {
    if( !Verbose )
        return 0;

    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);

    return ret;
}

int vverbose(const char * restrict format, ...) {
    if ( !StrongVerbose )
        return 0;

    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);

    return ret;
}