#include"../include/ev_tcpServer.h"
//==========初始化静态成员对象=====================
MyDB *ev_tcpServer::My_db=new MyDB();
car *ev_tcpServer::CarObject=new car;
MyJson *ev_tcpServer::my_json = new MyJson;
ofstream ev_tcpServer::server_log("1.jpg",ios::out|ios::binary);
int ev_tcpServer::WS_TCP_CON_ID = 0;
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
    init_watcher_data((watcher_data*)(rw_watcher->data),fd,sin.sin_port);
    // w_data->client_port = sin.sin_port;
    // ((watcher_data*)(rw_watcher->data))->image_id = 1;
    // ((watcher_data*)(rw_watcher->data))->user_id = 
    // ((watcher_data*)(rw_watcher->data))->fp.open()
    //==========判断链接是来自ne还是本地web=========================
    // if(sin.sin_port == 20108){
    //     ev_io_init(rw_watcher,recv_socket_cb_hardware,fd,EV_READ);
    // }
    if(sin.sin_addr.s_addr==inet_addr("127.0.0.1")){
        //本地ws与tcp的转换id
        WS_TCP_CON_ID = fd;
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
// void ev_tcpServer::recv_socket_cb_hardware(struct ev_loop *loop,ev_io *w, int revents)
// {
//     unsigned char data_buf[MAX_BUF_LEN] = {0};
//     int ret_len = 0;
//     int data_len=256;
//     char sql[200];
//     int port = ((watcher_data*)(w->data))->client_port;
//     do{
//         //获取小车底层数据
//         ret_len = recv(w->fd,data_buf,data_len, 0);
//         if(ret_len > 0)
//         {
//             //printf("%d,recv message:\n'%s'\n",w->fd,data_buf);
//             // for(int i=0;i<20;i++){
//             //     cout << hex << (short)data_buf[i] <<" ";
//             // }      
//             CarObject->DataReceive(data_buf,ret_len);
//             cout<<"解析之后："<<endl;
//             cout<<(short)CarObject->mode_data<<" "<<(short)CarObject->speed_data/7*0.1<<" "<<(short)CarObject->angle_data<<" "<<CarObject->soc_data<<endl;
//             //=============sql语句=============================
//             sprintf(sql,"insert into car_info(mode, speed,angle,soc,port) values('%f','%f','%f','%f','%d')ON DUPLICATE KEY UPDATE `port`='%d';"
//             ,(double)CarObject->mode_data,(double)CarObject->speed_data,(double)CarObject->angle_data,(double)CarObject->soc_data,port,port);
//             cout<<"sql:"<<sql<<endl;
//             if(My_db->exeSQL(sql,INSERT,My_db->result))
//             {
//                // printf("insert into db successfully! \n");
//             }
//             else {
//                 printf("fail to insert into db \n");
//             }
//              mysql_free_result(My_db->result);
//              My_db->result=NULL;
//              My_db->row=NULL;
//             return;
//         }
//         else{
//             printf("remote socket closed 2\n"); //客户端断开链接
//             break;
//         }
//         if(errno == EAGAIN ||errno == EWOULDBLOCK){
//             continue;
//         }
//         break;
//     }while(1);
//     //销毁链接、观察器
//     close(w->fd);
//     ev_io_stop(loop,w);
//     delete[] w;
// }
void ev_tcpServer::recv_socket_cb_local(struct ev_loop *loop,ev_io *w, int revents)
{
    printf("welcome to local recv!\n");
    char data_buf[MAX_BUF_LEN] = {0};
    unsigned char head_buf[HEAD_ALL_LEN];
    int ret_len = 0;
    int data_len=256;
    int socket_fd;
    char control_info[MAX_BUF_LEN+HEAD_ALL_LEN];
    do{
        //接收tcp流并解决粘包问题
        //1.获取报文首部
        ret_len = recv(w->fd,head_buf,HEAD_ALL_LEN,MSG_WAITALL);
        //cout<<"hex:"<<hex<<head_buf<<endl;
        if(ret_len==0||ret_len<0){
             printf("remote socket closed 1\n");
             break;
        }
        else {
                socket_fd = bytesToInt(head_buf+HEAD_TYPE_LEN,HEAD_CON_LEN);//前2字节是小车与tcp_server的连接id
                data_len = bytesToInt(head_buf+HEAD_CON_LEN+HEAD_TYPE_LEN,HEAD_DATA_LEN);//后四个字节为报文数据部分长度
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
            memcpy(control_info,head_buf,HEAD_ALL_LEN);
            memcpy(control_info+HEAD_ALL_LEN,data_buf,data_len); 
            int send_status = send(socket_fd,control_info,data_len+HEAD_ALL_LEN,MSG_NOSIGNAL);
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
}
void ev_tcpServer::recv_socket_cb_net(struct ev_loop *loop,ev_io *w, int revents)
{
    char data_buf[MAX_BUF_LEN] = {0};
    unsigned char head_buf[HEAD_ALL_LEN];
    char info[MAX_BUF_LEN+HEAD_ALL_LEN]={0};
    int ret_len = 0;
    int data_len=256;
    int recv_type = 0;
    char index[200];
    //cout<<"tcp"<<endl;
    // int port = ((watcher_data*)(w->data))->client_port;
   // bool res_flag=false;//回复客户端标志位 false：不用回复
    int user_status=-1;
    int ws_txt;
    int ws_photo;
    do{
        //1.获取报文首部
        ret_len = recv(w->fd,head_buf,HEAD_ALL_LEN,MSG_WAITALL);
        //断开连接
        if(ret_len==0||ret_len<0){
            //保存图片
            SaveImageToVideo(w);
            printf("remote socket closed 1\n");
            break; 
        }
        else {
            recv_type = bytesToInt(head_buf,HEAD_TYPE_LEN);
            data_len = bytesToInt(head_buf+HEAD_CON_LEN+HEAD_TYPE_LEN,HEAD_DATA_LEN);//后四个字节为报文数据部分长度
            cout<<"length:"<<data_len<<endl;                    
            for(int i = 0 ;i<HEAD_ALL_LEN;i++)
                cout<<(short)head_buf[i]<<endl;
        }
        //2.数据部分长度大于接收缓存的最大上限则主动关闭与client的链接
        if(data_len>MAX_BUF_LEN)
        {
            printf("buffer overflow \n");
            break;
        }      
        //3.获取数据部分
        ret_len = recv(w->fd,data_buf,data_len, MSG_WAITALL);
        user_status = Judge_user_offline(((watcher_data*)(w->data))->user_id,ws_txt,ws_photo);
        
        cout<<"recv_type:"<<recv_type<<endl;
        if(ret_len > 0)
        {                    
            if(recv_type==99){
                //存储用户图片
                //index=;
                //index.append((watcher_data*)(w->data))->passage_start_time)
                sprintf(index,"%s/%s/image%d.jpg",((watcher_data*)(w->data))->photo_dir.c_str(),((watcher_data*)(w->data))->passage_start_time.c_str(),((watcher_data*)(w->data))->image_id);
                cout<<index<<endl;
                ((watcher_data*)(w->data))->fp.open(index,ios::out|ios::binary);
                if(!((watcher_data*)(w->data))->fp) //直接这样判断
                {
                    cout<<"fail to open the file !\n"<<endl;
                }
                else{
                        ((watcher_data*)(w->data))->fp.write(data_buf,data_len);
                        ((watcher_data*)(w->data))->fp.close();
                        ((watcher_data*)(w->data))->image_id++;
                }
                if(user_status == OFFLINE){
                    //用户离线,不转发到web
                    return;
                }
                cout<<"WS_TCP_CON_ID:"<<WS_TCP_CON_ID<<endl;
                //给出ws客户端摄像头数据显示的id          
                memcpy(head_buf+HEAD_TYPE_LEN,intToBytes(ws_photo,HEAD_CON_LEN),HEAD_CON_LEN);
                //先发头部
                send(WS_TCP_CON_ID,head_buf,HEAD_ALL_LEN,MSG_NOSIGNAL);
                //再发数据部分
                send(WS_TCP_CON_ID,data_buf,data_len,MSG_NOSIGNAL);
                cout<<"转发摄像头数据！\n"<<endl;
                return ;
            }
            if(recv_type==100){
                if(user_status == OFFLINE){
                    //用户离线,不转发到web
                    return;
                }
                //给出ws客户端控制台数据显示的id  
                memcpy(head_buf+HEAD_TYPE_LEN,intToBytes(ws_txt,HEAD_CON_LEN),HEAD_CON_LEN);
                //先发头部
                send(WS_TCP_CON_ID,head_buf,HEAD_ALL_LEN,MSG_NOSIGNAL);
                //再发数据部分
                send(WS_TCP_CON_ID,data_buf,data_len,MSG_NOSIGNAL);
                //原数据返回
                printf("%d,recv message:\n'%s'\n",w->fd,data_buf);
                memcpy(info,head_buf,HEAD_ALL_LEN);
                memcpy(info+HEAD_ALL_LEN,data_buf,data_len);
                send(w->fd,info,data_len+HEAD_ALL_LEN,MSG_NOSIGNAL);
                return;
            }            
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
    printf("remote socket closed 3\n");
    //销毁链接
    close(w->fd);
    ev_io_stop(loop,w);
    delete[] w;
    //free(w);
}
void ev_tcpServer::write_socket_cb(struct ev_loop *loop,ev_io *w, int revents)
{
    char data_buf[MAX_BUF_LEN] = {0};
    char info[MAX_BUF_LEN+HEAD_ALL_LEN] ={0};  
    byte *head =new byte[HEAD_ALL_LEN];

    memset(head,0,sizeof(head));
    memcpy(data_buf,w->data,strlen(((watcher_data*)w->data)->data_buf));
    //创建报文首部长度(head 3-6字节存储长度)
    memcpy(head+2,intToBytes(strlen((data_buf))+1,4),4);
    //创建完整报文
    memcpy(info,head,HEAD_ALL_LEN);
    memcpy(info+HEAD_ALL_LEN,data_buf,strlen(data_buf)+1);
    int send_status=send(w->fd,info,strlen(data_buf)+HEAD_ALL_LEN+1,MSG_NOSIGNAL);
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
// int ev_tcpServer::deal_recv_info_net(ev_io *w,string data_buff,byte head[]){
//     int ws_id;
//     char info[MAX_BUF_LEN+HEAD_ALL_LEN];
//     int data_len = data_buff.length();
//     //cout<<"len:"<<data_len<<endl;
//     //将json数据解析
//     if(my_json->decodejson(data_buff)==RES_SUCC)
//     {
//         //cout<<"json:"<<my_json->action_type<<endl;
//         //if(my_json->action).compare()
//         switch(my_json->action_type){
//             case CAR_STATUS :{

//             }
//             case PLANPATH_RES :{
//                 //收到规划路径，转发给web端显示
//                 //从数据库获取ws_id
//                 ws_id = GetWs_connectID(w->fd);
//                 if(ws_id==RES_UNEXIST){
//                     printf("the ws_id isn't exist!\n");
//                     return RES_FAIL;
//                 }
//                 //cout<<"ws_id1:"<<ws_id<<endl;
//                 memcpy(head,intToBytes(ws_id,2),2);
//                // cout<<"head:"<<(short)head[0] << " " << (short)head[1] << endl;
//                 memcpy(head+2,intToBytes(data_len,4),4);
                
//                 //cout<<"ws_id4:"<<bytesToInt(head,2)<<endl;
//                 memcpy(info,head,HEAD_ALL_LEN);
//                 memcpy(info+HEAD_ALL_LEN,data_buff.c_str(),data_len);
//                 // for(int i=0;i<6;i++){
//                 //     cout<<hex<<(int)info[i]<<endl;
//                 // }
//                 send(WS_TCP_CON_ID,info,data_len+HEAD_ALL_LEN,MSG_NOSIGNAL);
//                 return RES_SUCC;
//             }
//             case PointMove_RES:{

//             }
//             case RosState_RES :{
//                 ws_id = GetWs_connectID(w->fd);
//                 if(ws_id==RES_UNEXIST){
//                     printf("the ws_id isn't exist!\n");
//                     return RES_FAIL;
//                 }
//                 cout<<"ws_id2:"<<ws_id<<endl;
//                 memcpy(head,intToBytes(ws_id,2),2);
//                 memcpy(info,head,HEAD_ALL_LEN);
//                 memcpy(info+HEAD_ALL_LEN,data_buff.c_str(),data_len);
//                 send(WS_TCP_CON_ID,info,data_len+HEAD_ALL_LEN,MSG_NOSIGNAL);
//                 return RES_SUCC;
//             }
//             case RosRealState_RES :{
//                 ws_id = GetWs_connectID(w->fd);
//                 if(ws_id==RES_UNEXIST){
//                     printf("the ws_id isn't exist!\n");
//                     return RES_FAIL;
//                 }
//                 cout<<"ws_id3:"<<ws_id<<endl;
//                 memcpy(head,intToBytes(ws_id,2),2);
//                 memcpy(head+2,intToBytes(data_len,4),4);
//                 memcpy(info,head,HEAD_ALL_LEN);
//                 memcpy(info+HEAD_ALL_LEN,data_buff.c_str(),data_len);
//                 send(WS_TCP_CON_ID,info,data_len+HEAD_ALL_LEN,MSG_NOSIGNAL);
//                 return RES_SUCC;
//             }
//             case CONTRAL_RES :{

//             }
//             case ArriveCrossing_RES :{

//             }
//             defualt:return UKONOW_ERROR;
//         }

//     }else{
//             printf("catch an error when decoding json!\n");
//     }

// }
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
        strftime(tmpBuf, 255, "%Y-%m-%d^%H:%M:%S", localtime(&t)); //format date and time.
        return tmpBuf; 
}
int ev_tcpServer::Judge_user_offline(int user_id,int &ws_txt,int &ws_photo){
    char sql[200];
    sprintf(sql,"select ws_txt,ws_photo from user_info where user_id ='%d' and ws_status = '1'",user_id);
    cout<<sql<<endl;
    if(My_db->exeSQL(sql,SELECT,My_db->result))
    {
        My_db->row=mysql_fetch_row(My_db->result);
        ws_txt=atoi(My_db->row[0]);
        ws_photo=atoi(My_db->row[1]);
        cout<<"ws_txt:"<<ws_txt<<endl;
        cout<<"ws_photo:"<<ws_photo<<endl;
        mysql_free_result(My_db->result);
        My_db->result=NULL;
        My_db->row=NULL;
        return ONLINE;   
    }
    else {
        //mysql_free_result(My_db->result);
        My_db->result=NULL;
        My_db->row=NULL;
        return OFFLINE;
    }
     //cout<<"ws_id:"<<ws_id<<endl;
    
}

int ev_tcpServer::GetUserID(int socket_fd){
    char sql[200];
    int user_id;
    sprintf(sql,"select user_id from user_info where tcp_id ='%d'",socket_fd);
    cout<<sql<<endl;
    if(My_db->exeSQL(sql,SELECT,My_db->result))
    {
        My_db->row=mysql_fetch_row(My_db->result);
        user_id=atoi(My_db->row[0]);
        mysql_free_result(My_db->result);
        My_db->result=NULL;
        My_db->row=NULL;
        return user_id;   
    }
    else {
        //mysql_free_result(My_db->result);
        My_db->result=NULL;
        My_db->row=NULL;
        return RES_UNEXIST;
    }
     //cout<<"ws_id:"<<ws_id<<endl;  
}
int ev_tcpServer::init_watcher_data(struct watcher_data *w_data,int socket_fd ,int port){
    int user_id = GetUserID(socket_fd);
    char dir_index[100];
    cout<<"user_id"<<user_id<<endl;
    
    if(RES_UNEXIST == user_id)
    {
        printf("user isn't exist!\n");
        return RES_UNEXIST;
    }

    w_data->client_port = port;
    w_data->image_id = 1;
    w_data->user_id = user_id;
    w_data->passage_start_time=GetTime();
    //创建用户专属目录
    sprintf(dir_index,"/var/www/html/web/user_info/user_%d",user_id);
    mkdir(dir_index,0777);
    //图片目录
    sprintf(dir_index,"/var/www/html/web/user_info/user_%d/photo",user_id);
    w_data->photo_dir = dir_index;
    mkdir(dir_index,0777);
    //视频目录
    sprintf(dir_index,"/var/www/html/web/user_info/user_%d/video",user_id);
    w_data->video_dir = dir_index;
    mkdir(dir_index,0777);

    //创建当前时间下的视频与图片文件夹
    // sprintf(dir_index,"/var/www/html/web/user_info/user_%d/video/%s",user_id,w_data->passage_start_time.c_str());
    // mkdir(dir_index,0777);
    sprintf(dir_index,"/var/www/html/web/user_info/user_%d/photo/%s",user_id,w_data->passage_start_time.c_str());
    mkdir(dir_index,0777);
    //sprintf(index,"%d/image%d",user_id,)
    //w_data->fp.open()
}
 void ev_tcpServer::SaveImageToVideo(ev_io *w){
    FILE *make_video_fp;
    char make_video_ord[200];
    char sql[200];
    char index[100];
    sprintf(make_video_ord,"ffmpeg -r 10 -i %s/%s/image%%d.jpg -vcodec h264 %s/%s.mp4",
    ((watcher_data*)(w->data))->photo_dir.c_str(),((watcher_data*)(w->data))->passage_start_time.c_str(),((watcher_data*)(w->data))->video_dir.c_str(),((watcher_data*)(w->data))->passage_start_time.c_str());
    cout<<make_video_ord<<endl;
    make_video_fp=popen(make_video_ord,"r");            
    pclose(make_video_fp);
    //判断是否成功生成视频
    sprintf(index,"%s/%s.mp4",((watcher_data*)(w->data))->video_dir.c_str(),((watcher_data*)(w->data))->passage_start_time.c_str());
    if(-1 == access(index, F_OK)) 
    {
        cout<<"video file isn's exist!\n"<<endl;
    }
    else{
            sprintf(sql,"insert into video_info(user_id,video_name) value('%d','%s.mp4')",
            ((watcher_data*)(w->data))->user_id,((watcher_data*)(w->data))->passage_start_time.c_str());
                cout<<"sql:"<<sql<<endl;
            if(My_db->exeSQL(sql,INSERT,My_db->result))
            {
            // printf("insert into db successfully! \n");
            }
            else{
                printf("fail to insert into db \n");
            }
            mysql_free_result(My_db->result);
            My_db->result=NULL;
            My_db->row=NULL;
            ((watcher_data*)(w->data))->fp.close();
    }


    //更新用户数据库
    
 }