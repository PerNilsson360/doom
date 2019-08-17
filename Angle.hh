#ifndef _ANGLE_HH_
#define _ANGLE_HH_

#include <cmath>
#include <cstdio>
#include <cassert>
#include <ostream>

#include "doomdef.h"

class Vertex;

class Angle
{
public:
    Angle(double angle = 0.0) {
        truncate(angle);
    }
    Angle(const Vertex& v1, const Vertex& v2);
    Angle(double x1, double y1, double x2, double y2);
    Angle(dirtype_t);
    Angle& operator += (const Angle& a) {
        truncate(_angle + a._angle);
        return *this;
    }
    Angle& operator -= (const Angle& a) {
        truncate(_angle - a._angle);
        return *this;
    }
    // Unary minus
    Angle operator - () const {
        return Angle(-_angle);
    }
    bool operator == (const Angle& a) const {
        return _angle == a._angle;
    }
    bool operator >= (const Angle& a) const {
        return _angle >= a._angle;
    }
    bool operator != (const Angle& a) const {
        return _angle != a._angle;
    }
    bool operator > (const Angle& a) const {
        return _angle > a._angle;
    }
    bool operator < (const Angle& a) const {
        return _angle < a._angle;
    }
    operator double() const {
        return _angle;
    }
    operator dirtype_t() const {
	Angle tmp(_angle + (Angle::A45 / 2));
	return (dirtype_t)((double)tmp / Angle::A45);
    }
    void roundToDir() {
	*this = Angle((dirtype_t) *this);
    }
    // @todo these should Angle's not double I guess
    static const double A0;
    static const double A5;
    static const double A30;
    static const double A45;
    static const double A90;
    static const double A180;
    static const double A270;
    static const double A360;
private:
    void truncate(double angle) {
        if (angle >= A360) {
            _angle = fmod(angle, A360);
        } else if (angle < 0) {
            _angle = angle;
            while (_angle < 0)
                _angle += A360;
        }else {
            _angle = angle;
        }
	if (_angle == A360) {
	    _angle = 0;
	}
        assert(_angle >= 0 && _angle < A360); 
    }        
    double _angle;
};

// Since Angle does support negative angles a double is returned
inline double operator - (const Angle& left, const Angle& right) {
    // @todo remove cast when angle_t is completly removed 
    return (double)left - (double)right;
}

inline Angle operator + (const Angle& left, const Angle& right) {
    // @todo remove cast when angle_t is completly removed 
    return Angle((double)left + (double)right);
}

inline Angle operator / (const Angle& num, int den) {
    // @todo remove cast when angle_t is completly removed 
    return Angle((double)num / den);
}

inline Angle operator * (const Angle& a, int s) {
    // @todo remove cast when angle_t is completly removed 
    return Angle((double)a * s);
}

inline Angle operator * (int s, const Angle& a) {
    // @todo remove cast when angle_t is completly removed 
    return Angle((double)a * s);
}

inline std::ostream& operator << (std::ostream& os, const Angle& angle) {
    os << (double)angle;
    return os;
}

inline double diff (const Angle& left, const Angle& right) {
    return fabs(left -right);
}

#endif
