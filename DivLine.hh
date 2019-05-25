#ifndef _DIV_LINE_HH_
#define _DIV_LINE_HH_

#include "r_defs.h"

class DataInput;

class DivLine
{
public:
    DivLine();
    DivLine(double x1, double x2, double y1, double y2);
    DivLine(const line_t& li);
    DivLine(const DataInput& dataInput);
    DivLine(const mapnode_t& mapNode);
    int moveX(double fraction);
    int moveY(double fraction);
    bool isChangeBiggerThan(double d);
    bool isPositive() const;
    int pointOnSide(double x, double y);
    double interceptVector(const DivLine& divLine);
private:
    double x;
    double y;
    double dx;
    double dy;    
};

#endif
