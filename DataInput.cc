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


std::string
DataInput::readEightBytes() const
{
    std::string result;
    static const int n = 8;
    if (_i + n > _length) {
	std::stringstream ss;
	ss << "DataInput::readEightBytes overflow : " << _i
	   << " length: " << _length << std::endl;
	throw std::runtime_error(ss.str());
    }
    for (int j = 0; j < n; j++) {
        char c = _data[_i + j];
	if (c == 0) break;
	result += c;
    }
    _i += n;
    return result;
}
