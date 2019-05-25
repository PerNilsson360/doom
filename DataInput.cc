#include <sstream>
#include <stdexcept>

#include "DataInput.hh"

DataInput::DataInput(const unsigned char* data, int length) :
    _length(length), _i(0), _data(data)
{
}

short
DataInput::readShort() const
{
    static const int n = 2;
    if (_i + n > _length) {
	std::stringstream ss;
	ss << "DataInput::readShort overflow : " << _i
	   << " length: " << _length << std::endl;
	throw std::runtime_error(ss.str());
    }
    union {
        short s;
        char c[n];
    } conv;
    for (int j = 0; j < n; j++) {
        conv.c[j] = _data[_i + j];
    }
    _i += n;
    return conv.s;
}
