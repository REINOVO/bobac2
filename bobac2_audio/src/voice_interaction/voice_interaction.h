#include <vector>
#include <ros/ros.h>
#include <move_base_msgs/MoveBaseAction.h>
#include "bobac2_audio/Collect.h"
#include "bobac2_audio/Awake.h"
#include "bobac2_audio/Mode.h"
#include "bobac2_audio/Control.h"
#include "bobac2_audio/Nav.h"
#include "bobac2_audio/AIUI.h"
#include "bobac2_audio/TTS.h"

using namespace std;

class Interaction
{
public:
	Interaction();
	~Interaction();
    
private:
    int i, position_count;
	bool wakeup_flag;
	int mode_choose_flag;
	string aplay_file;	

	ros::NodeHandle m_handle;
	ros::Publisher vcmd_pub;
	ros::Publisher cancel_pub;
	ros::ServiceClient collect_client;
	ros::ServiceClient awake_client;
	ros::ServiceClient mode_client;
	ros::ServiceClient control_client;
	ros::ServiceClient nav_client;
	ros::ServiceClient aiui_client;
	ros::ServiceClient tts_client;

	bobac2_audio::Collect collect_srv;	
	bobac2_audio::Awake awake_srv;
	bobac2_audio::Mode mode_srv;
	bobac2_audio::Control control_srv;
	bobac2_audio::Nav nav_srv;
	bobac2_audio::AIUI aiui_srv;
	bobac2_audio::TTS  tts_srv;
	actionlib_msgs::GoalID first_goal;
	void run();

};
