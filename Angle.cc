#include <cmath>
#include <cassert>

#include "Angle.hh"


const double Angle::A0(0.0);
const double Angle::A5(M_PI/36);
const double Angle::A45(M_PI/4);
const double Angle::A90(M_PI/2);
const double Angle::A180(M_PI);
const double Angle::A270(M_PI + M_PI/2);
const double Angle::A360(2 * M_PI);

Angle::Angle(double x1, double y1, double x2, double y2)
{
    double x = x2 - x1;
    double y = y2 - y1;
    if (y >= 0) { 
        if (x == 0) {
            _angle = A90;
        } else {
            if (x > 0) {
                _angle = atan(y/x);
            } else {
                x *= -1;
                _angle = A180;
                _angle -= atan(y/x);
            }
        }
    } else {        
        if (x == 0) {
            _angle = A270;
        } else {
            y *= -1;
            if (x < 0) {
                x *= -1;
                _angle = A180;
                _angle += atan(y/x);
            } else {
                _angle = A360;
                _angle -= atan(y/x);
            }
        }
    }
    if (_angle == A360) {
	_angle = 0;
    }
    assert(not(_angle < 0));
}

Angle::Angle(dirtype_t direction)
{
    if (direction >= DI_NODIR) {
	assert(false && "Angle(dirtype_t) angle is DI_NODIR");
    }
    // Direction is from 0 - 7 and 0 is east (polar cordinates)
    _angle = (M_PI/4) * direction;
}
