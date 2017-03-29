#pragma once
#include <stdio.h>

// converts the depth floats into a color BMP, saved to fOut
void outputBmp(FILE* fOut, const float* const pflDepths);