#include"../include/MyJson.h"
 MyJson::MyJson(){
    action = "";
    serial = "";
    result = "";//返回状态，常量定义为：成功=“0000”，失败=“0001”
    msg= "";//描述
    maxCarSpeed = 0;//到达此地图点的最大车速
    x = 0,y= 0,heading = 0;//坐标点,角度
    //vector<PlanPath_node> PlanPath_info={0,0,0};//规划路径上经过的所有节点
    action_type = 0;//依据ACTION，给出action_type(宏定义)
    user_id = 0;
 }
 MyJson::~MyJson(){

 }
int MyJson::decodejson(string json_data){
    Json::Reader reader;
    Json::Value value;
    PlanPath_node one_node;
    try{
         if(reader.parse(json_data,value)){
            if(value["action"].isNull()){
                    return ACT_NULL;
                }
                else {
                    //依据action进行json解析
                   // memcpy(action,value["action"].asString(),(value["action"].asString().length())
                    action=value["action"].asString();
                    //1
                    if(action.compare("link_photo")==0)
                    {
                        action_type = LINK_PHOTO;
                        user_id = value["user_id"].asInt();
                        cout<<"user_id"<<user_id<<endl;
                        //cout<<"json suc!"<<endl;
                        return RES_SUCC;                           
                    }
                     if(action.compare("link_txt")==0)
                    {
                        action_type = LINK_TXT;
                        user_id = value["user_id"].asInt();
                        cout<<"user_id"<<user_id<<endl;
                        //cout<<"json suc!"<<endl;
                        return RES_SUCC;                           
                    }
                    //2
                    if(action.compare("lamp_open")==0)
                    {
                        action_type = LAMP_OPEN;
                        return RES_SUCC;      
                     }
                     //3
                     if(action.compare("lamp_close")==0)
                     {
                        action_type = LAMP_CLOSE;
                        //cout<<"json suc!"<<endl;
                        return RES_SUCC;      
                     }                   
                }
            }
    }
    catch(exception &e){
        cout<<e.what()<<endl;
        return CATCH_ERROR;
    }
    return RES_SUCC;
    } 