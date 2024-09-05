#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <fstream>

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>

using namespace std;

extern bool DEBUGGING;

extern void handler(int sig);

#endif