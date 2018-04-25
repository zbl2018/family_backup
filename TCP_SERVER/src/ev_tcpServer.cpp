#include"../include/ev_tcpServer.h"
//==========初始化静态成员对象=====================
MyDB *ev_tcpServer::My_db=new MyDB();
car *ev_tcpServer::CarObject=new car;
MyJson *ev_tcpServer::my_json = new MyJson;
ofstream ev_tcpServer::server_log("1.jpg",ios::out|ios::binary);
//==============================================
ev_tcpServer::ev_tcpServer(string db_ip,string db_user_name,string db_pwd,string db_name){
    //初始化并连接数据库
    if(!My_db->initDB(db_ip,db_user_name,db_pwd,db_name))
    {
        printf("fail to init DB!\n");
        exit(-1);//初始化数据库失败
    }

}
ev_tcpServer::~ev_tcpServer(){

    }

int ev_tcpServer::start_server(int port)
{
    //创建socket描述符
    int socket_fd = create_socket(port);
    if(socket_fd < 0){
        return 0;
    }
    //接收链接观察器
    ev_io accept_watcher;
    //设置epoll轮询模式	
    struct ev_loop *loop = ev_loop_new(EVBACKEND_EPOLL);
    //初始化观察器
    ev_io_init(&accept_watcher, accept_socket_cb,socket_fd, EV_READ);
    //将观察器注册进轮询周期中
    ev_io_start(loop,&accept_watcher); 
    //开始轮询
    ev_loop(loop,0);
    //销毁
    ev_loop_destroy(loop);
    return 1;
} 
int ev_tcpServer::create_socket(int port)
{
    struct sockaddr_in addr;
    int s;
    s = socket(AF_INET, SOCK_STREAM, 0);

    if(s == -1){
        perror("create socket error \n");
        return -1;
    }
    //开启端口复用
    int so_reuseaddr = 1;
    setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&so_reuseaddr,sizeof(so_reuseaddr));
    bzero(&addr, sizeof(addr));
    addr.sin_family = PF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr =htonl(INADDR_ANY);
    //addr.sin_addr.s_addr = inet_addr(listen_ip.c_str());
    //绑定监听地址与端口
    if(bind(s, (struct sockaddr *) &addr, sizeof(struct sockaddr))== -1){
        perror("bind socket error \n");
        return -1;
    }
    if(listen(s,32) == -1){
        perror("listen socket error\n");
        return -1;
    }
    printf("bind this computer,listen %d \n",port);
    return s;
}
void ev_tcpServer::accept_socket_cb(struct ev_loop *loop,ev_io *w, int revents)
{
    int fd; 
    int s = w->fd;
    struct sockaddr_in sin;
    socklen_t addrlen = sizeof(struct sockaddr);
    do{
        fd = accept(s, (struct sockaddr *)&sin, &addrlen);
        if(fd > 0){
            break;
        }

        if(errno == EAGAIN || errno == EWOULDBLOCK){
            continue;
        }
    }while(1);
    ev_io* rw_watcher = new ev_io;
    memset(rw_watcher,0x00,sizeof(ev_io));
    //自定义观察器用户数据结构
    rw_watcher->data=new watcher_data;
    ((watcher_data*)(rw_watcher->data))->client_port = sin.sin_port;
    // std::cout<<"aaa:"<<sin.sin_addr.s_addr<<endl;
    // cout<<"1111:"<<inet_addr("127.0.0.1")<<endl;
    //==========判断链接是来自底层还是来自net_node、本地web=========================
    if(sin.sin_port == 20108){
        ev_io_init(rw_watcher,recv_socket_cb_hardware,fd,EV_READ);
    }
    if(sin.sin_addr.s_addr==inet_addr("127.0.0.1")){
        ev_io_init(rw_watcher,recv_socket_cb_local,fd,EV_READ);
    } 
    else {
        ev_io_init(rw_watcher,recv_socket_cb_net,fd,EV_READ);
    }
    ev_io_start(loop,rw_watcher);
    printf("accept successfully!\n");
    printf("the port of client is %d and its id is:%d\n", ((watcher_data*)(rw_watcher->data))->client_port,fd);
    //sleep(3);  
}

void ev_tcpServer::recv_socket_cb_hardware(struct ev_loop *loop,ev_io *w, int revents)
{
    unsigned char data_buf[MAX_BUF_LEN] = {0};
    int ret_len = 0;
    int data_len=256;
    char sql[200];
    int port = ((watcher_data*)(w->data))->client_port;
    do{
        //获取小车底层数据
        ret_len = recv(w->fd,data_buf,data_len, 0);
        if(ret_len > 0)
        {
            //printf("%d,recv message:\n'%s'\n",w->fd,data_buf);
            // for(int i=0;i<20;i++){
            //     cout << hex << (short)data_buf[i] <<" ";
            // }      
            CarObject->DataReceive(data_buf,ret_len);
            cout<<"解析之后："<<endl;
            cout<<(short)CarObject->mode_data<<" "<<(short)CarObject->speed_data/7*0.1<<" "<<(short)CarObject->angle_data<<" "<<CarObject->soc_data<<endl;
            //=============sql语句=============================
            sprintf(sql,"insert into car_info(mode, speed,angle,soc,port) values('%f','%f','%f','%f','%d')ON DUPLICATE KEY UPDATE `port`='%d';"
            ,(double)CarObject->mode_data,(double)CarObject->speed_data,(double)CarObject->angle_data,(double)CarObject->soc_data,port,port);
            cout<<"sql:"<<sql<<endl;
            if(My_db->exeSQL(sql,INSERT,My_db->result))
            {
               // printf("insert into db successfully! \n");
            }
            else {
                printf("fail to insert into db \n");
            }
             mysql_free_result(My_db->result);
             My_db->result=NULL;
             My_db->row=NULL;
            return;
        }
        else{
            printf("remote socket closed 2\n"); //客户端断开链接
            break;
        }
        if(errno == EAGAIN ||errno == EWOULDBLOCK){
            continue;
        }
        break;
    }while(1);
    //销毁链接、观察器
    close(w->fd);
    ev_io_stop(loop,w);
    delete[] w;
}
void ev_tcpServer::recv_socket_cb_local(struct ev_loop *loop,ev_io *w, int revents)
{
    printf("welcome to local recv!\n");
    char data_buf[MAX_BUF_LEN] = {0};
    unsigned char head_buf[HEAD_LEN];
    int ret_len = 0;
    int data_len=256;
    int socket_fd;
    char control_info[MAX_BUF_LEN+HEAD_LEN];
    do{
        //接收tcp流并解决粘包问题
        //1.获取报文首部
        ret_len = recv(w->fd,head_buf,HEAD_LEN,MSG_WAITALL);
        //cout<<"hex:"<<hex<<head_buf<<endl;
        if(ret_len==0||ret_len<0){
             printf("remote socket closed 1\n");
             break;
        }
        else {
                socket_fd = bytesToInt(head_buf,2);//前2字节是小车与tcp_server的连接id
                data_len = bytesToInt(head_buf+2,4);//后四个字节为报文数据部分长度
            }
        cout<<"socket_fd:"<<socket_fd<<endl;
        cout<<"length:"<<data_len<<endl;
        //2.数据部分长度大于接收缓存的最大上限则主动关闭与client的链接
        if(data_len>MAX_BUF_LEN||data_len<0)
        {
            printf("buffer overflow \n");
            break;
        }
        //3.获取数据部分
        ret_len = recv(w->fd,data_buf,data_len, MSG_WAITALL);
        if(ret_len > 0)
        {
            printf("%d,recv message:\n'%s'\n",w->fd,data_buf);
            //控制小车
            memcpy(control_info,head_buf,HEAD_LEN);
            memcpy(control_info+HEAD_LEN,data_buf,data_len); 
            int send_status = send(socket_fd,control_info,data_len+HEAD_LEN,MSG_NOSIGNAL);
            if(send_status){
                printf("%d bytes send control_info successfully!\n",send_status);
            }else{
                printf("fail to send control_info!\n");
            }     
            return;
        }
        else{
            printf("remote socket closed 2\n"); //客户端断开链接
            break;
        }
        if(errno == EAGAIN ||errno == EWOULDBLOCK){
            continue;
        }
        break;
    }while(1);
    //销毁链接
    close(w->fd);
    ev_io_stop(loop,w);
    delete[] w;
    //free(w);
}
void ev_tcpServer::recv_socket_cb_net(struct ev_loop *loop,ev_io *w, int revents)
{
    char data_buf[MAX_BUF_LEN] = {0};
    unsigned char head_buf[HEAD_LEN];
    char info[MAX_BUF_LEN+6]={0};
    int ret_len = 0;
    int data_len=256;
    int recv_type = 0;
    cout<<"tcp"<<endl;
    // char sql[200];
    // int port = ((watcher_data*)(w->data))->client_port;
    bool res_flag=false;//回复客户端标志位 false：不用回复
    do{
                //1.获取报文首部
                ret_len = recv(w->fd,head_buf,HEAD_LEN,MSG_WAITALL);
                //cout<<"hex:"<<hex<<head_buf<<endl;
                if(ret_len==0||ret_len<0){
                    printf("remote socket closed 1\n");
                    break;
                }
                else data_len = bytesToInt(head_buf+2,4);//后四个字节为报文数据部分长度
                cout<<"length:"<<data_len<<endl;
                
                for(int i = 0 ;i<6;i++)
                    cout<<(short)head_buf[i]<<endl;
                //2.数据部分长度大于接收缓存的最大上限则主动关闭与client的链接
                if(data_len>MAX_BUF_LEN)
                {
                    printf("buffer overflow \n");
                    break;
                }
                recv_type = bytesToInt(head_buf,2);
                //3.获取数据部分
                ret_len = recv(w->fd,data_buf,data_len, MSG_WAITALL);
                if(ret_len > 0)
                {
                    //net_node那边并没有加终结符
                    //data_buf[data_len]='\0';
                    if(recv_type==99){
                        //server_log.open("1.jpg");
                        server_log.write(data_buf,data_len);
                        memcpy(head_buf,intToBytes(1,2),2);
                        memcpy(head_buf+2,intToBytes(data_len,4),4);
                        memcpy(info,head_buf,HEAD_LEN);
                        memcpy(info+HEAD_LEN,data_buf,data_len);
                        send(WS_TCP_CON_ID,info,data_len+HEAD_LEN,MSG_NOSIGNAL);
                        return ;
                    }
                    printf("%d,recv message:\n'%s'\n",w->fd,data_buf);
                    memcpy(info,head_buf,HEAD_LEN);
                    memcpy(info+HEAD_LEN,data_buf,data_len);
                    send(w->fd,info,data_len+HEAD_LEN,MSG_NOSIGNAL);
                    //res_flag=deal_recv_info_net(w,data_buf,head_buf);
                    //回复客户端
                    // if(res_flag)
                    // {
                    //     //遵循先读后写的流程，下一次轮询将监听写事件，回复net_node
                    //     ev_io_stop(loop,w);
                    //     ev_io_init(w,write_socket_cb,w->fd,EV_WRITE);
                    //     ev_io_start(loop,w);
                    //     return;
                    // }
                    // else {
                    //     //不用回复客户端，继续接收客户端信息
                    // }            
                    return;
                }
                else{
                    printf("remote socket closed 2\n"); //客户端断开链接
                    break;
                }
                if(errno == EAGAIN ||errno == EWOULDBLOCK){
                    continue;
                }
                break;
               // }  
    }while(1);
    //销毁链接
    close(w->fd);
    ev_io_stop(loop,w);
    delete[] w;
    //free(w);
}
void ev_tcpServer::write_socket_cb(struct ev_loop *loop,ev_io *w, int revents)
{
    char data_buf[MAX_BUF_LEN] = {0};
    char info[MAX_BUF_LEN+HEAD_LEN] ={0};  
    byte *head =new byte[HEAD_LEN];

    memset(head,0,sizeof(head));
    memcpy(data_buf,w->data,strlen(((watcher_data*)w->data)->data_buf));
    //创建报文首部长度(head 3-6字节存储长度)
    memcpy(head+2,intToBytes(strlen((data_buf))+1,4),4);
    //创建完整报文
    memcpy(info,head,HEAD_LEN);
    memcpy(info+HEAD_LEN,data_buf,strlen(data_buf)+1);
    int send_status=send(w->fd,info,strlen(data_buf)+HEAD_LEN+1,MSG_NOSIGNAL);
    if(send_status<0){
        printf("fail to response client! The socket id is %d ",w->fd);
        //sleep(1);
    }

    delete[] head;
    //遵循先读后写的流程，下一次轮询将监听读事件
    ev_io_stop(loop,w);
    ev_io_init(w,recv_socket_cb_net,w->fd,EV_READ);
    ev_io_start(loop,w);
}
int ev_tcpServer::deal_recv_info_net(ev_io *w,string data_buff,byte head[]){
    int ws_id;
    char info[MAX_BUF_LEN+HEAD_LEN];
    int data_len = data_buff.length();
    //cout<<"len:"<<data_len<<endl;
    //将json数据解析
    if(my_json->decodejson(data_buff)==RES_SUCC)
    {
        //cout<<"json:"<<my_json->action_type<<endl;
        //if(my_json->action).compare()
        switch(my_json->action_type){
            case CAR_STATUS :{

            }
            case PLANPATH_RES :{
                //收到规划路径，转发给web端显示
                //从数据库获取ws_id
                ws_id = GetWs_connectID(w->fd);
                if(ws_id==RES_UNEXIST){
                    printf("the ws_id isn't exist!\n");
                    return RES_FAIL;
                }
                //cout<<"ws_id1:"<<ws_id<<endl;
                memcpy(head,intToBytes(ws_id,2),2);
               // cout<<"head:"<<(short)head[0] << " " << (short)head[1] << endl;
                memcpy(head+2,intToBytes(data_len,4),4);
                
                //cout<<"ws_id4:"<<bytesToInt(head,2)<<endl;
                memcpy(info,head,HEAD_LEN);
                memcpy(info+HEAD_LEN,data_buff.c_str(),data_len);
                // for(int i=0;i<6;i++){
                //     cout<<hex<<(int)info[i]<<endl;
                // }
                send(WS_TCP_CON_ID,info,data_len+HEAD_LEN,MSG_NOSIGNAL);
                return RES_SUCC;
            }
            case PointMove_RES:{

            }
            case RosState_RES :{
                ws_id = GetWs_connectID(w->fd);
                if(ws_id==RES_UNEXIST){
                    printf("the ws_id isn't exist!\n");
                    return RES_FAIL;
                }
                cout<<"ws_id2:"<<ws_id<<endl;
                memcpy(head,intToBytes(ws_id,2),2);
                memcpy(info,head,HEAD_LEN);
                memcpy(info+HEAD_LEN,data_buff.c_str(),data_len);
                send(WS_TCP_CON_ID,info,data_len+HEAD_LEN,MSG_NOSIGNAL);
                return RES_SUCC;
            }
            case RosRealState_RES :{
                ws_id = GetWs_connectID(w->fd);
                if(ws_id==RES_UNEXIST){
                    printf("the ws_id isn't exist!\n");
                    return RES_FAIL;
                }
                cout<<"ws_id3:"<<ws_id<<endl;
                memcpy(head,intToBytes(ws_id,2),2);
                memcpy(head+2,intToBytes(data_len,4),4);
                memcpy(info,head,HEAD_LEN);
                memcpy(info+HEAD_LEN,data_buff.c_str(),data_len);
                send(WS_TCP_CON_ID,info,data_len+HEAD_LEN,MSG_NOSIGNAL);
                return RES_SUCC;
            }
            case CONTRAL_RES :{

            }
            case ArriveCrossing_RES :{

            }
            defualt:return UKONOW_ERROR;
        }

    }else{
            printf("catch an error when decoding json!\n");
    }

}
int ev_tcpServer::deal_recv_info_hardware(ev_io *w,string data_buff)
{ 
    //switch()//在此根据接收的数据类型判断接下来该进行的操作
    //返回客户端的数据在w->data保存   
    /*=================执行数据库sql语句例子========================================*/
    // if(My_db->exeSQL("select port from car_info",SELECT,My_db->result))
    // {
    //     //列数
    //     int num_fields = mysql_num_fields(My_db->result);
    //     //行数
    //     while((My_db->row=mysql_fetch_row(My_db->result)))
    //     {
    //         //mysql_fetch_field(My_db.row)
    //         for(int i=0;i<num_fields;i++)
    //         {
    //             if(My_db->row[i]==w->data->port){
    //                     return PORT_EXIST;
    //             }
                   
    //             cout<<My_db->row[i]<<" ";
    //         }
    //         cout<<endl;
    //     }
    // }
    //  mysql_free_result(My_db->result);
    //  My_db->result=NULL;
    //  My_db->row=NULL;
}
int ev_tcpServer::deal_send_info(ev_io *w,string data_buff){
    Json::Value root;
	Json::FastWriter writer;
	root["key"]=1111;
	root["name"]="zbl";
	string strValue= writer.write(root);//root.toStyledString();
}


byte* ev_tcpServer::intToBytes(int value,int byte_len){
       if(byte_len>4||byte_len<1)
    {
       cout<<"byte_len overflow!"<<endl;
        return 0;           
    }
    byte *des = new byte[byte_len]; 
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
int ev_tcpServer::bytesToInt(byte* des, int byte_len){
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
string ev_tcpServer::GetTime(){
        time_t t = time( 0 );   
        char tmpBuf[255];   
        strftime(tmpBuf, 255, "%Y-%m-%d %H:%M:%S", localtime(&t)); //format date and time.
        return tmpBuf; 
}
int ev_tcpServer::GetWs_connectID(int this_car_tcpId){
    char sql[200];
    int ws_id;
    sprintf(sql,"select ws_id from rel_user_car where tcp_id ='%d'",this_car_tcpId);
    cout<<sql<<endl;
    if(My_db->exeSQL(sql,SELECT,My_db->result))
    {
        My_db->row=mysql_fetch_row(My_db->result);
        ws_id=atoi(My_db->row[0]);
        mysql_free_result(My_db->result);
        My_db->result=NULL;
        My_db->row=NULL;
        return ws_id;   
    }
    else {
        //mysql_free_result(My_db->result);

        My_db->result=NULL;
        My_db->row=NULL;
        return RES_UNEXIST;
    }
     //cout<<"ws_id:"<<ws_id<<endl;
    
}
// string ev_tcpServer::decodejson(string json_data ,int type){ 
//     Json::Reader reader;
//     Json::Value value;
//      if(reader.parse(json_data,value)){
//         if(value["action"].isNull()){
//             return JSON_NULL;
//         }
//     }
//     //string js;
//     switch(type){
//         case 1: return value["action"].asString();
//         case 2: return value["serial"].asString();
//         default : return JSON_NULL;
//     } 
// }