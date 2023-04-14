#include <string.h>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

/**
 * 调整 tcpServer.cc 97 行创建 TimeWheel 的后两个参数
 * client connect 后，等待一段时间再发消息，时间轮到期后自动 clear 该连接
*/
int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    string ip = "127.0.0.1";
    uint16_t port = 9000;

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);

    int rt = connect(fd, (sockaddr *)&addr, sizeof(addr));
    if(rt != 0) {
        cout << "connect error" << endl;
        return 0;
    }

    
    string msg;
    cout << "please input: ";
    cin >> msg;
    int n = write(fd, msg.c_str(), msg.size());
    if(n > 0) {
        cout << "send success !" << endl;
    } else {
        cout << "send failed !" << endl;
    }

    char buf[1024] = {0};
    n = read(fd, buf, sizeof(buf));
    if(n < 0) {
        cout << "recv failed !" << endl;
    } else {
        cout << "recv success ! n = " << n << ", msg = " << buf << endl;
    }


    return 0;
}