#ifndef PARSE_H
#define PARSE_H

#include <stdlib.h>
#include <stdio.h>
#include <and_or_tree.h>

#define NS 10
#define NA 11
#define NC 10

#define NI 14


char *INFO[NI] = {"f1","f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12", "f13", "f14"};
char *SENSOR[NS] = {"laser", "camera", "comm", "sonar", "dummy", "timecost"};
char *ATOMIC[NA] = {"ps1", "ps2", "ps3","ps4","ps5", "ps13", "cs", "ms1", "ms2", "ms4", "ms14"};
char *COMPOSITE[NC] = {"clear", "clear1", "clear2"};

int numrobot = 0;

#endif
