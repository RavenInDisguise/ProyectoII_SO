#pragma once
#ifndef __STDBOOL_H
#include <stdbool.h>
#endif

int verbose(const char * restrict, ...);
void setVerbose(bool);