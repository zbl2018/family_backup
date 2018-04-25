#include <iostream>
#include "websocketpp/config/asio_no_tls.hpp"
#include "websocketpp/server.hpp"
#include"MyJson.h"
#include"MyDB.h"
typedef websocketpp::server<websocketpp::config::asio> WebsocketServer;
typedef WebsocketServer::message_ptr message_ptr;
typedef WebsocketServer::connection_ptr connection_ptr;
//using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using namespace std;
#define byte unsigned char
//WS同时存在最大的最大连接数
#define MAX_WSCON 65535
#define HEAD_LEN 6
#define MAX_BUF_LEN 999999
class ws_server{
    public:
            //websocket
            static void OnOpen(WebsocketServer *server, websocketpp::connection_hdl hdl);
            static void OnClose(WebsocketServer *server, websocketpp::connection_hdl hdl);
            static void OnMessage(WebsocketServer *server, websocketpp::connection_hdl hdl, message_ptr msg);

            void init_ws_server(string db_ip,string db_user_name,string db_pwd,string db_name);
            void ws_run(int port);
            //tcp_client
            void InitTcpClient_ConnectServer(string ip_addr,int tcp_server_port);
            static int reconnect_tcp_server(int &clientSocket, struct sockaddr_in serverAddr);
            static void *recvInfo_from_TcpServer(void* args);
            //TOOL
            static bool ImmediateTranspond(int tcp_id,string msg);                 
            static byte* intToBytes(int value,int byte_len);
            static int bytesToInt(byte* des, int byte_len);
            static int GetWsId(websocketpp::connection_hdl hdl);
            string GetTime();      
    private:
             WebsocketServer  server;
             static sockaddr_in serverAddr;
             static int clientSocket_fd;
             //连接id键值对
             static map<int,websocketpp::connection_hdl>connect_id;
             static int loc_id;
             static vector<int>OffLine_id;
             //tcp接收线程的id
             pthread_t recv_th_id;
             static MyDB *My_db;
             //static MyJson *my_json;
};
        