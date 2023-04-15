#ifndef _ABSTRACTCODEC_H
#define _ABSTRACTCODEC_H

#include <memory>

#include "buffer.h"
#include "abstractData.h"

namespace util {

enum ProtocalType {
    HTTP = 1,
    TCP = 2,
};

class AbstractCodec {
public:
    typedef std::shared_ptr<AbstractCodec> ptr;

    AbstractCodec() {}
    virtual ~AbstractCodec() {}

    virtual void encode(Buffer *buf, AbstractData *data) = 0;
    virtual void decode(Buffer *buf, AbstractData *data) = 0;

    virtual ProtocalType getProtocalType() = 0;
};

}   // namespace util

#endif
