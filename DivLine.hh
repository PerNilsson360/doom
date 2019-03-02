#ifndef _DIV_LINE_HH_
#define _DIV_LINE_HH_

#include "r_defs.h"

class DivLine
{
public:
    DivLine();
    DivLine(const line_t& li);
    int pointOnSide(double x, double y);
    double interceptVector(const DivLine& divLine);
//private:
    double x;
    double y;
    double dx;
    double dy;    
};

#endif
