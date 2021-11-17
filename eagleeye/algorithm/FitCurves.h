#ifndef _FITCURVES_H_
#define _FITCURVES_H_
#include "eagleeye/algorithm/GraphicsGems.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <vector>

typedef Point2 *BezierCurve;

/* Forward declarations */
void FitCurve(Point2* d, 
              int nPts, 
              float error, 
              std::vector<float>& control_points, 
              std::vector<int>& regions);

#endif