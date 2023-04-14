#ifndef _ABSTRACTDATA_H
#define _ABSTRACTDATA_H

namespace util {

class AbstractData {
public:
    AbstractData() : decode_succ(false), encode_succ(false) {};
    virtual ~AbstractData() {}

    bool decode_succ;
    bool encode_succ;
};

}   // namespace util

#endif
