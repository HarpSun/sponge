#include "socket.hh"
#include "util.hh"

#include <cstdlib>
#include <iostream>

using namespace std;

void get_URL(const string &host, const string &path) {
    // Your code here.

    // You will need to connect to the "http" service on
    // the computer whose name is in the "host" string,
    // then request the URL path given in the "path" string.

    // Then you'll need to print out everything the server sends back,
    // (not just one call to read() -- everything) until you reach
    // the "eof" (end of file).

    // 1 创建 socket 对象
    // 2 socket.connect() 连接远程服务器
    // 3 socket.send() 发送 HTTP 报文
    // 4 获取响应并输出

    TCPSocket socket;
    // http 默认 80 端口
    socket.connect(Address(host, "http"));

    // 用下面的方式来格式化字符串
    char httpString[100];
    snprintf(httpString,
             sizeof(httpString),
             "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", path.c_str(), host.c_str());

    socket.write(httpString);
    // SHUT_WR 表示之后这个 socket 只能收数据 不能再发送了
    socket.shutdown(SHUT_WR);

    string response;
    int size = 1024;
    while (not socket.eof()) {
        response += socket.read(size);
    }
    cout << response;
}

int main(int argc, char *argv[]) {
    try {
        if (argc <= 0) {
            abort();  // For sticklers: don't try to access argv[0] if argc <= 0.
        }

        // The program takes two command-line arguments: the hostname and "path" part of the URL.
        // Print the usage message unless there are these two arguments (plus the program name
        // itself, so arg count = 3 in total).
        if (argc != 3) {
            cerr << "Usage: " << argv[0] << " HOST PATH\n";
            cerr << "\tExample: " << argv[0] << " stanford.edu /class/cs144\n";
            return EXIT_FAILURE;
        }

        // Get the command-line arguments.
        const string host = argv[1];
        const string path = argv[2];

        // Call the student-written function.
        get_URL(host, path);
    } catch (const exception &e) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
