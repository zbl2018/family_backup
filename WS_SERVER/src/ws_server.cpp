//#include "stdafx.h"
#include"../include/ws_server.h"

//=============静态变量初始化=================
int ws_server::clientSocket_fd =0 ;
int ws_server::loc_id= 1;
map<int,websocketpp::connection_hdl> ws_server::connect_id;
vector<int> ws_server::OffLine_id;
sockaddr_in ws_server::serverAddr = {AF_INET,22222,inet_addr("127.0.0.1")};
MyDB *ws_server::My_db=new MyDB();
//MyJson *ws_server::my_json = new MyJson;
////////////////////////////////////////////

void ws_server::OnOpen(WebsocketServer *server, websocketpp::connection_hdl hdl)
{
    cout << "have client connected"<< endl;
    cout <<"ptr:"<<hdl.lock().get()<<"  loc_id"<<loc_id<<endl;   
    if(loc_id<MAX_WSCON){
        connect_id.insert(make_pair(loc_id,hdl));//记录连接id
        loc_id++;
    }
    else{
        //重用连接id（已经断开的id）
    }
   
}

void ws_server::OnClose(WebsocketServer *server, websocketpp::connection_hdl hdl)
{
    char sql[100];
    cout << "have client disconnected" << endl;
    int ws_id = GetWsId(hdl);
    sprintf(sql,"update user_info set ws_status = '0' where ws_txt = '%d' or ws_photo = '%d'",ws_id,ws_id);
    My_db->exeSQL(sql,UPDATE,My_db->result);
    int off_id=GetWsId(hdl);//保存离线id
    OffLine_id.push_back(off_id);
    connect_id.erase(off_id);
    //更新数据库
    
}

void ws_server::OnMessage(WebsocketServer *server, websocketpp::connection_hdl hdl, message_ptr msg)
{
    byte head[HEAD_ALL_LEN];
    char send_info[1024]={0};
    string strMsg = msg->get_payload();
    MyJson *my_json = new MyJson();
    char sql[100];
    cout << strMsg << endl;
    //默认arm与服务器的tcp连接 id 为 8
    if(RES_SUCC == my_json->decodejson(strMsg))
    {
        cout<<"json"<<endl;
        switch(my_json->action_type)
        {
            case LINK_PHOTO :{
                    int ws_photo = GetWsId(hdl);
                    cout<<"ws_photo:"<<ws_photo<<endl;
                    sprintf(sql,"update user_info set ws_photo ='%d',ws_status = '1' where user_id = '%d'",ws_photo,my_json->user_id);
                    My_db->exeSQL(sql,UPDATE,My_db->result);
                    return;     
            }
            case LINK_TXT :{                   
                    int ws_txt = GetWsId(hdl);
                    cout<<"ws_txt:"<<ws_txt<<endl;
                    sprintf(sql,"update user_info set ws_txt ='%d',ws_status = '1' where user_id = '%d'",ws_txt,my_json->user_id);
                    My_db->exeSQL(sql,UPDATE,My_db->result); 
                    return ;        
            }
            case LAMP_OPEN :{
                    ImmediateTranspond(8,strMsg);
                    return ;     
            }
            case LAMP_CLOSE :{
                    ImmediateTranspond(8,strMsg);
                    return ;     
            }
            default :{
                    ImmediateTranspond(8,strMsg);
                    cout<<"default"<<endl;
                    return ;  
            }
        }
    }
else {
        printf("web端发送来的数据格式有错误！\n");
        //server->close(hdl,1);
    }
    //connection_ptr con = server->get_con_from_hdl(hdl);
    //byte temp[6];
    //deal infomatiom of head  
    string strRespon = "receive: ";
    strRespon.append(strMsg);
    server->send(hdl.lock(), strRespon, websocketpp::frame::opcode::text);
    delete my_json;
}

void ws_server::init_ws_server(string db_ip,string db_user_name,string db_pwd,string db_name){

    if(!My_db->initDB(db_ip,db_user_name,db_pwd,db_name))
    {
        printf("fail to init DB!\n");
        exit(-1);//初始化数据库失败
    }
    // Set logging settings
    server.set_access_channels(websocketpp::log::alevel::all);
    server.clear_access_channels(websocketpp::log::alevel::frame_payload);
    // Initialize ASIO
    server.init_asio();

    // Register our open handler
    server.set_open_handler(bind(&OnOpen, &server, ::_1));

    // Register our close handler
    server.set_close_handler(bind(&OnClose, &server, _1));

    // Register our message handler
    server.set_message_handler(bind(&OnMessage, &server, _1, _2));
}
void ws_server::ws_run(int port){
    server.set_reuse_addr(true);
    server.listen(port);
    //Start the server accept loop
    server.start_accept();
    printf("start listen port: %d in websocket\n",port);
    //Start the ASIO io_service run loop
    server.run();
}
void ws_server::InitTcpClient_ConnectServer(string ip_addr,int tcp_server_port){

    serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(tcp_server_port);
	serverAddr.sin_addr.s_addr = inet_addr(ip_addr.c_str());
	//链接远程主机
	reconnect_tcp_server(clientSocket_fd,serverAddr);
    //与tcp_server连接成功，创建接收线程
    int res = pthread_create(&recv_th_id,NULL,recvInfo_from_TcpServer,&server);
    if(res==0){
        printf("create recv pthread successfully!\n");
    }
    else {
         printf("fail to create recv pthread!\n");
    }

}
int ws_server::reconnect_tcp_server(int &clientSocket_fd, struct sockaddr_in serverAddr){
	int is_connect=-1;
	//创建句柄	
	if((clientSocket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return 1;
	}
	//链接	
	is_connect=connect(clientSocket_fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	if(is_connect<0)
		{
			perror("connect");
			printf("wait for connect server...\n");
		}	
	while(is_connect<0){
		is_connect=connect(clientSocket_fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	}
	printf("connect with destination host...\n");
	return 1;
}
void * ws_server::recvInfo_from_TcpServer(void* args){
        WebsocketServer *server = (WebsocketServer*)args;
		char data_buf[MAX_BUF_LEN];
		unsigned char head_buf[HEAD_ALL_LEN];
		int ret_len;
        int data_len;
        int ws_id;
        int recv_type;
        websocketpp::connection_hdl ws_hdl;
	while(1){
        // memset(data_buf,'\0',MAX_BUF_LEN);
        // memset(head_buf,0,HEAD_LEN);
       
		ret_len = recv(clientSocket_fd,head_buf,HEAD_ALL_LEN,MSG_WAITALL);
        //cout<<"hex:"<<hex<<head_buf<<endl;
        if(ret_len==0||ret_len<0){
             printf("remote socket closed 1\n");
             close(clientSocket_fd);
             reconnect_tcp_server(clientSocket_fd,serverAddr);
        }
        else{
            // for(int i=0;i<6;i++){
            //         //cout<<hex<<(short)head_buf[i]<<endl;
            //     }
            recv_type = bytesToInt(head_buf,HEAD_TYPE_LEN);
            ws_id=bytesToInt(head_buf+HEAD_TYPE_LEN,HEAD_CON_LEN);
            data_len = bytesToInt(head_buf+HEAD_TYPE_LEN+HEAD_CON_LEN,HEAD_DATA_LEN);//后四个字节为报文数据部分长度 
        }
        cout<<"length:"<<data_len<<endl;
        cout<<"ws_id:"<<ws_id<<endl;
        //2.数据部分长度大于接收缓存的最大上限则主动关闭与client的链接
        if(data_len>MAX_BUF_LEN)
        {
            printf("buffer overflow \n");
            close(clientSocket_fd);
            reconnect_tcp_server(clientSocket_fd,serverAddr);
        }
        //3.获取数据部分,转发给web端
        ret_len = recv(clientSocket_fd,data_buf,data_len, MSG_WAITALL);
        if(ret_len > 0){
            //转发
            //printf("%d,recv message:\n'%s'\n",clientSocket_fd,data_buf);
            if(connect_id.count(ws_id))
            {
                ws_hdl = connect_id[ws_id];
            }else{
                printf("the ws_id:%d isn't in db\n",ws_id);
                continue;
            }
            if(recv_type==99)
                server->send(ws_hdl, data_buf,data_len,websocketpp::frame::opcode::BINARY);
            else server->send(ws_hdl, data_buf,data_len,websocketpp::frame::opcode::TEXT);
        }
        else{
            printf("remote socket closed 2\n"); //客户端断开链接
            close(clientSocket_fd);
            reconnect_tcp_server(clientSocket_fd,serverAddr);
        }
	}
	
}
byte* ws_server::intToBytes(int value,int byte_len){
    if(byte_len>4||byte_len<1)
    {
       cout<<"byte_len overflow!"<<endl;
        return 0;
    }
    byte *des = new byte[byte_len]; 
    // for(int i = 0; i < byte_len; i++)
    // {
    //     des[i] = (byte)((value >> ((byte_len - i - 1) * 8)) & 0xFF);
    // }
    switch(byte_len){
            case 4:{ 
                //四位
                des[3] = (byte) (value & 0xff);
                des[2] = (byte) ((value >> 8) & 0xff);
                des[1] = (byte) ((value >> 16) & 0xff);
                des[0] = (byte) ((value >> 24) & 0xff); 
                break;
            }    
            case 3:{
                //三位
                des[2] = (byte) (value & 0xff);
                des[1] = (byte) ((value >>8) & 0xff);
                des[0] = (byte) ((value >>16) & 0xff); 
                break;
            }
        case 2:{
                //二位
                des[1] = (byte) (value& 0xff);
                des[0] = (byte) ((value >>8) & 0xff); 
                break;   
        }
        case 1:{
                //一位
                des[0] = (byte) (value & 0xff);
                break; 
        }      
        }  
        return des;  
}
int ws_server::bytesToInt(byte* des, int byte_len){
        if(byte_len>4||byte_len<1){
        cout<<"byte_len overflow!"<<endl;
        return 0;           
        }  
        int value;    
        switch(byte_len){
            case 4:{ 
                //四位
                value = (int)((des[3] & 0xff)  
                | ((des[2] & 0xff) << 8)  
                | ((des[1] & 0xff) << 16)  
                | (des[0] & 0xff) << 24);
                break;
            }    
            case 3:{
                //三位
                value = (int) ((des[2] & 0xff)  
                | ((des[1] & 0xff) << 8)  
                | ((des[0] & 0xff) << 16));
                break;
            }
        case 2:{
                //二位
                value = (int) ((des[1] & 0xff)  
                | ((des[0] & 0xff) << 8));
                break;   
        }
        case 1:{
                //一位
                value = (int) ((des[0] & 0xff));
                break; 
        }      
        }       
        return value;  
    }  
string ws_server::GetTime(){
        time_t t = time( 0 );   
        char tmpBuf[255];   
        strftime(tmpBuf, 255, "%Y-%m-%d %H:%M:%S", localtime(&t)); //format date and time.
        return tmpBuf; 
}
int ws_server::GetWsId(websocketpp::connection_hdl hdl){
    map<int,websocketpp::connection_hdl>::iterator it;
    it = connect_id.begin();
    while(it != connect_id.end())
    {
        //it->first;
        if(it->second.lock().get()==hdl.lock().get()){
            return it->first;
        }       
        else it ++;         
    }
}
bool ws_server::ImmediateTranspond(int tcp_id,string msg){
    byte head_buf[HEAD_ALL_LEN];
    memcpy(head_buf+HEAD_TYPE_LEN,intToBytes(tcp_id,2),2);
    memcpy(head_buf+HEAD_TYPE_LEN+HEAD_CON_LEN,intToBytes(msg.length(),HEAD_DATA_LEN),HEAD_DATA_LEN);
    int send_status =-1;
    // cout<<"id:"<<bytesToInt(head_buf,2)<<endl;    
    // cout<<"headlen:"<<bytesToInt(head_buf+2,4)<<endl;
    send_status=send(clientSocket_fd, head_buf,HEAD_ALL_LEN, MSG_NOSIGNAL);
    if(send_status>0){
        printf("send head_buf to tcp server successfully!\n");
    }
    else{
        printf("fail to send head_buf to tcp server!\n");
    }
    //memcpy(send_info,head,6);
    //memcpy(send_info+6,strMsg.c_str(),strMsg.length());
    send_status=send(clientSocket_fd, msg.c_str(),msg.length(), MSG_NOSIGNAL);
    if(send_status>0){
        printf("send data to tcp server successfully!\n");
    }
    else{
        printf("fail to send data to tcp server!\n");
    }
}