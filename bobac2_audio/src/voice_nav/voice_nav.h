#include <iostream>
#include <vector>
#include <thread>
 
#include "bobac2_audio/qisr.h"
#include "bobac2_audio/msp_cmn.h"
#include "bobac2_audio/msp_errors.h"

#include "ros/ros.h"
#include "bobac2_audio/Nav.h"
#include "bobac2_audio/TTS.h"
#include <move_base_msgs/MoveBaseAction.h>
#include <actionlib/client/simple_action_client.h>
#define BUFFER_SIZE     4096
#define FRAME_LEN       640 
#define HINTS_SIZE  100

using namespace std;

class VoiceNav 
{
public:
	VoiceNav();
	~VoiceNav();

	ros::NodeHandle m_handle;

	void send_goal(int pnum);
	void nav_cancel();
	void state_detection();
	void tarvel();

	bool send_flag;
	bool cancel_flag;
	bool travel_flag;

	vector<string> position_name;
	vector<double> position_x;
	vector<double> position_y;
	vector<double> orientation_z;
	vector<double> orientation_w;
	vector<string> position_introduction;
	
	void login();
	void logout();
	void position_read();
	string run_iat(const char* audio_file);
	
	
	ros::ServiceServer voice_nav_service;
	ros::ServiceClient tts_client;
	bobac2_audio::TTS  tts_srv;
	move_base_msgs::MoveBaseGoal goal;
	bool voice_nav_deal(bobac2_audio::Nav::Request &req,bobac2_audio::Nav::Response &res);
	actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> *ac;
	typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> AC;
private:
	thread travel_thread;
};
