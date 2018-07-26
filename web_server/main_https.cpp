#include "server_https.hpp"
#include "handler.hpp"
using namespace skWeb;

int main(){
    //HTTPS服务运行在12345端口，并启用四个线程
    Server<HTTPS> server(12345, 4, "server.crt", "server.key");
    start_server<Server<HTTPS>>(server);
    return 0;
}
