#ifndef _DATAINPUT_HH_
#define _DATAINPUT_HH_

class DataInput
{
public:
    DataInput(const unsigned char* data, int length);
    short readShort() const;
private:
    int _length;
    mutable int _i;
    const unsigned char* _data;
};

#endif
