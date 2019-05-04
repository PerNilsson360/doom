#ifndef _MAP_THING_HH_
#define _MAP_THING_HH_


// Thing definition, position, orientation and type,
// plus skill/visibility flags and attributes.
class MapThing
{
public:
    double getAngle();
    // private: //@todo
    short		x;
    short		y;
    short		angle;
    short		type;
    short		options;
};

#endif
