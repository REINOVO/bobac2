#include <ros/ros.h>
#include <bobac2_audio/AIUI.h>
#include "AIUITest.h"
#include "FileUtil.h"
using namespace std;

#define TEST_ROOT_DIR "./voice_config/AIUI/"

//配置文件打的路径，里面是客户端设置的参数
#define CFG_FILE_PATH "./voice_config/AIUI/cfg/aiui.cfg"

#define GRAMMAR_FILE_PATH "./voice_config/AIUI/asr/call.bnf"

#define LOG_DIR "./voice_config/AIUI/log"

#define VAD_JET_DIR "./voice_config/AIUI/assets/vad/meta_vad_16k.jet"

#define APPID "5d536221"

AIUITester* t=NULL;  
bool aiui_cb(bobac2_audio::AIUI::Request &req, bobac2_audio::AIUI::Response &res){
    switch(req.mode){
        case false:
            printf("\033[47;33m音频输入模式\033[0m\n");
        case true:
            printf("\033[47;33m文本输入模式\033[0m\n");  
    }
    t->test(req.mode, req.text.c_str());
    res.iat_text = t->ret_iat;
    res.nlp_text = t->ret_nlp;
    if(res.iat_text.empty()){
       ROS_ERROR(" No semantics detected !");
       res.nlp_text = "不好意思，没有听清楚";
    }
    std::cout<<"<<<<<<<"<<res.nlp_text<<">>>>>>>"<<std::endl;
    return true;
}
 int main(int argc, char *argv[])
{   
	ros::init(argc, argv, "voice_aiui");
	ros::NodeHandle nh;
    string appid = "5d536221";
	ros::ServiceServer aiui_server = nh.advertiseService("voice_aiui", aiui_cb);
        printf("\033[47;33mmode为true时，text读取的是文本\033[0m\n");
        printf("\033[47;33mmode为false时，text读取的是音频\033[0m\n");
	t = new AIUITester(TEST_ROOT_DIR, LOG_DIR, VAD_JET_DIR);
    t->createAgent(CFG_FILE_PATH, APPID);
    t->wakeup();
	ros::spin();
	delete t;
	return 0;
}
