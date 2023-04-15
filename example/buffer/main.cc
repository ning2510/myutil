#include "buffer.h"

#include <iostream>
#include <memory>
#include <functional>

using namespace std;
using namespace util;

int main() {
    shared_ptr<Buffer> buf = std::make_shared<Buffer>();

    string str = "123456";
    buf->append(str.c_str(), str.size());

    int32_t tmp = 1;
    buf->appendInt32(tmp);

    string prestr = "000";
    buf->prepend(prestr.c_str(), prestr.size());

    int readAble = buf->readableBytes();
    string result = buf->retrieveAsString(readAble - 4);

    int32_t resultInt32 = buf->peekInt32();

    cout << "result = " << result << endl;
    cout << "resultInt32 = " << resultInt32 << endl;

    return 0;
}