#pragma once
#include <iostream>
#include <string>
#include <stdarg.h>
#include <malloc.h>
#include <Windows.h>
#include "paDemo.h"
using namespace std;



void initMwLog(paDemo* mainwindow);


void writeLogT(LPCTCH fmt, ...);
void writeLogA(LPCCH fmt, ...);






