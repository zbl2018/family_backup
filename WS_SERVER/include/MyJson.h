#include<string.h>
#include<vector>
#include <string>
#include<map>
#include <exception>  
#include"json/json.h"
using namespace std;
//#define JSON_NULL "_NULL"
#define RES_FAIL 1001
#define RES_SUCC 1002
#define ACT_NULL 1003
#define SUCC "0000"
#define FAIL "0001"
#define CATCH_ERROR 1004
#define UKONOW_ERROR 10000
#define LINK_SUCC 21
#define LAMP_OPEN 22
#define LAMP_CLOSE 23
#define LINK_PHOTO 24
#define LINK_TXT 25
struct PlanPath_node{
        double x,y,z;
        int no;//规划路径返回的点的序号
        string id;//任务点的序号
};
class MyJson{
    public:
            int decodejson(string json_data);
            MyJson();
            ~MyJson();
            string action;
            string serial;
            string result;//返回状态，常量定义为：成功=“0000”，失败=“0001”
            string msg;//描述
            double maxCarSpeed;//到达此地图点的最大车速
            double x,y,heading;//坐标点,角度
            vector<PlanPath_node> PlanPath_info;//规划路径上经过的所有节点
            int action_type;//依据ACTION，给出action_type(宏定义)
            int user_id;
           // map<string,void>CarSstatus;//小车的状态
};
