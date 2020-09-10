#include "voice_interaction.h"
string word = "到达"  ;
Interaction::Interaction()
{
	wakeup_flag = false;
	mode_choose_flag = 0;
	cancel_pub = m_handle.advertise<actionlib_msgs::GoalID>("move_base/cancel",1);

	collect_client = m_handle.serviceClient<bobac2_audio::Collect>("voice_collect");
	awake_client = m_handle.serviceClient<bobac2_audio::Awake>("voice_awake");
	mode_client = m_handle.serviceClient<bobac2_audio::Mode>("voice_mode");
	control_client = m_handle.serviceClient<bobac2_audio::Control>("voice_control");
	nav_client = m_handle.serviceClient<bobac2_audio::Nav>("voice_nav");
	aiui_client = m_handle.serviceClient<bobac2_audio::AIUI>("voice_aiui");
	tts_client = m_handle.serviceClient<bobac2_audio::TTS>("voice_tts");
	run();
}

Interaction::~Interaction()
{

}

void Interaction::run()
{
	ros::Rate loop(10);
	while(ros::ok()){
		collect_srv.request.collect_flag = true;
		while(!wakeup_flag){
			if(collect_client.call(collect_srv)){
				awake_srv.request.voice_filename = collect_srv.response.voice_filename;
				if(awake_client.call(awake_srv)){
					if(awake_srv.response.awake_flag){
						wakeup_flag = true;
                                                system("aplay  ./voice_config/voice_awake/awake.wav");
					}
					else cout<< " 休眠中，请用唤醒词唤醒。" << endl;
				}
			}
		}
		ROS_INFO("mode_flag : %d", mode_choose_flag);
		if(mode_choose_flag == 0){
			wakeup_flag = false;            
			if(collect_client.call(collect_srv)){  
				aiui_srv.request.mode = false;          
				aiui_srv.request.text = collect_srv.response.voice_filename;
				if(!aiui_client.call(aiui_srv)){
					cout<<"aiui response get failed"<<endl;
				}
				if(!aiui_srv.response.nlp_text.empty() || !aiui_srv.response.iat_text.empty()){
					if(aiui_srv.response.iat_text.find("切换模式") != string::npos){
						mode_choose_flag = 3;
						wakeup_flag = true;
						tts_srv.request.text = "请选择要切换的模式"; 
						if(tts_client.call(tts_srv)) {
							system("aplay  ./voice_config/voice_tts/tts.wav");
						}
					}
					else{
						tts_srv.request.text = aiui_srv.response.nlp_text; 
						if(tts_client.call(tts_srv)) {
							system("aplay  ./voice_config/voice_tts/tts.wav");
						}
					}
				}
			}
		}
		if(mode_choose_flag == 1){ 
			if(collect_client.call(collect_srv)){
				wakeup_flag = false;            
				control_srv.request.voice_filename = collect_srv.response.voice_filename;
				if(!control_client.call(control_srv)){
					cout<<"--control order get failed!--"<<endl;
				}
				else if(control_srv.response.control_order.find("切换模式") != string::npos){					
					mode_choose_flag = 3;
					wakeup_flag = true;
					tts_srv.request.text = "请选择要切换的模式"; 
					if(tts_client.call(tts_srv)) {
						system("aplay  ./voice_config/voice_tts/tts.wav");
					}
				}
			}
			else 
				ROS_ERROR("voice collected failed");
		}
		if(mode_choose_flag == 2){//导航模式
			if(collect_client.call(collect_srv)){
				wakeup_flag = false; 
				nav_srv.request.voice_filename = collect_srv.response.voice_filename;
				if(!nav_client.call(nav_srv)){
					cout<<"voice nav error"<<endl;
				}
				else if(nav_srv.response.position == "取消导航"){
					cancel_pub.publish(first_goal);
				}
				else if(nav_srv.response.position.find("切换模式") != string::npos){					
					mode_choose_flag = 3;
					wakeup_flag = true;
					tts_srv.request.text = "请选择要切换的模式"; 
					if(tts_client.call(tts_srv)) {
						system("aplay  ./voice_config/voice_tts/tts.wav");
					}
				}

			}
		}
		
		if(mode_choose_flag == 3){
			if(collect_client.call(collect_srv)){  
				mode_srv.request.voice_filename = collect_srv.response.voice_filename;
				if(!mode_client.call(mode_srv)){
					cout<<"--mode choose error!--"<<endl;
				}	
				if(mode_srv.response.mode == "控制模式"){
					tts_srv.request.text = "进入控制模式"; 
					if(tts_client.call(tts_srv)) {
						system("aplay  ./voice_config/voice_tts/tts.wav");
					}
					mode_choose_flag = 1;
				}
				else if(mode_srv.response.mode == "导航模式"){
					tts_srv.request.text = "进入导航模式"; 
					if(tts_client.call(tts_srv)) {
						system("aplay  ./voice_config/voice_tts/tts.wav");
					}
					mode_choose_flag = 2;
				}
				else if(mode_srv.response.mode == "聊天模式"){
					tts_srv.request.text = "进入聊天模式"; 
					if(tts_client.call(tts_srv)) {
						system("aplay  ./voice_config/voice_tts/tts.wav");
					}
					mode_choose_flag = 0;
				}
				else if(mode_srv.response.mode.find("切换模式") != std::string::npos){
					mode_choose_flag = 3;
					tts_srv.request.text = "请选择要切换的模式"; 
					if(tts_client.call(tts_srv)) {
						system("aplay  ./voice_config/voice_tts/tts.wav");
					}
				}
				else{
					cout<<"不好意思，模式未能识别"<<endl;
					mode_choose_flag = 3;
					tts_srv.request.text = "不好意思，模式未能识别"; 
					if(tts_client.call(tts_srv)) {
						system("aplay  ./voice_config/voice_tts/tts.wav");
					}
				}
			}
		}
	}

}


int main(int argc,char** argv)
{
	ros::init(argc,argv,"voice_interaction");	
	Interaction interaction;
	ros::spin();

}
