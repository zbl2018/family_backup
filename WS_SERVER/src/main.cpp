#include"../include/ws_server.h"
int main()
{
    ws_server WS;
    WS.InitTcpClient_ConnectServer("127.0.0.1",22222);
    WS.init_ws_server("127.0.0.1","root","4817","web");
    //Listen on port 20000
    WS.ws_run(20000);

    return 0;
}