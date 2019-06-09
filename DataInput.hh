#ifndef _DATAINPUT_HH_
#define _DATAINPUT_HH_

#include <string>

class DataInput
{
public:
    DataInput(const unsigned char* data, int length);
    short readShort() const;
    std::string readEightBytes() const;
private:
    int _length;
    mutable int _i;
    const unsigned char* _data;
};

#endif
