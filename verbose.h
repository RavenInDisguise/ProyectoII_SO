#pragma once
#ifndef __STDBOOL_H
#include <stdbool.h>
#endif

int verbose(const char * restrict, ...);
int vverbose(const char * restrict, ...);
void setVerbose(bool);
void setStrongVerbose(bool);