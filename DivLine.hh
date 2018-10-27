#ifndef _DIV_LINE_HH_
#define _DIV_LINE_HH_

class DivLine
{
public:
    int pointOnSide(double x, double y);
    double interceptVector(const DivLine& divLine);
//private:
    double x;
    double y;
    double dx;
    double dy;    
};

#endif
