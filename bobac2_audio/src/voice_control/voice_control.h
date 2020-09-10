#include "ros/ros.h"
#include "geometry_msgs/Twist.h"
#include "bobac2_audio/Control.h"

#include <bobac2_audio/qisr.h>　
#include <bobac2_audio/msp_cmn.h> 
#include <bobac2_audio/msp_errors.h> 

#define	BUFFER_SIZE	4096
#define FRAME_LEN	640 
#define HINTS_SIZE  100

#define LOGIN_PARAMS  "appid = 5d536221, work_dir = ./voice_config/voice_control"//登录参数,appid与msc库绑定,请勿随意改动
#define SESSION_BEGIN_PARAMS  "sub = iat, domain = iat, language = zh_cn, accent = mandarin, sample_rate = 16000, result_type = plain, result_encoding = utf8"
	/*
	* sub:				请求业务类型
	* domain:			领域
	* language:			语言
	* accent:			方言
	* sample_rate:		音频采样率
	* result_type:		识别结果格式
	* result_encoding:	结果编码格式
	*
	*/
//可以不用选择上传用户词表
class Control{
public:
    Control();
    ~Control();
    void login(const char* login_params);
	void logout();
	void position_read();
	std::string run_iat(const char* audio_file, const char* session_begin_params);
private:
	ros::NodeHandle nh;
	ros::ServiceServer control_server;
    ros::Publisher vel_pub;
	bool control_cb(bobac2_audio::Control::Request &req,bobac2_audio::Control::Response &res);
};
