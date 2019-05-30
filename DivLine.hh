#ifndef _DIV_LINE_HH_
#define _DIV_LINE_HH_

#include "r_defs.h"

class DataInput;

class DivLine
{
public:
    DivLine();
    DivLine(const Vertex& v1, const Vertex& v2);
    DivLine(double x1, double x2, double y1, double y2);
    DivLine(const line_t& li);
    DivLine(const DataInput& dataInput);
    int moveX(double fraction) const;
    int moveY(double fraction)const;
    bool isChangeBiggerThan(double d) const;
    bool isPositive() const;
    int pointOnSide(double x, double y) const;
    int pointOnSide(const Vertex& vertex) const;
    double interceptVector(const DivLine& divLine) const;
private:
    double x;
    double y;
    double dx;
    double dy;    
};

#endif
