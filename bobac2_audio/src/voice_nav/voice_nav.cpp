#include "voice_nav.h"

VoiceNav::VoiceNav():cancel_flag(false),travel_flag(true),send_flag(false)
{
    login();
    position_read();
    
    voice_nav_service = m_handle.advertiseService("voice_nav",&VoiceNav::voice_nav_deal,this);
	tts_client = m_handle.serviceClient<bobac2_audio::TTS>("voice_tts");
    ac = new AC("move_base", true);
    
}
VoiceNav::~VoiceNav()
{
    if(travel_thread.joinable()){
        travel_thread.join();
    }
	delete ac;
    logout();

}

void VoiceNav::position_read()
{
    FILE* fp;
    char str[100];
    double tmp;

    if((fp = fopen("./voice_config/voice_nav/position_info.txt", "r")) < 0){
        cout<<"open position info  file  error！！"<<endl;
    }
    while(1){	
        if(fscanf(fp, "%s", str) == EOF)
            break;
        position_name.push_back(str);

        if(fscanf(fp,"%lf",&tmp) == EOF)
            break;
        position_x.push_back(tmp);

        if(fscanf(fp,"%lf",&tmp) == EOF)
            break;
        position_y.push_back(tmp);

        if(fscanf(fp,"%lf",&tmp) == EOF)
            break;
        orientation_z.push_back(tmp);

        if(fscanf(fp,"%lf",&tmp) == EOF)
            break;
        orientation_w.push_back(tmp);
        if(fscanf(fp,"%s",str) == EOF)
            break;
		position_introduction.push_back(str);
    }
    fclose(fp);

}

void VoiceNav::login()
{
    int ret = MSP_SUCCESS;
    const char* login_params = "appid = 5d536221, work_dir = ./voice_config/voice_nav"; // 登录参数，appid与msc库绑定,请勿随意改动
    /* 用户登录 */
    ret = MSPLogin(NULL, NULL, login_params); //第一个参数是用户名，第二个参数是密码，均传NULL即可，第三个参数是登录参数    
    if (MSP_SUCCESS != ret)
    {
        printf("AwakeLogin failed , Error code %d.\n",ret);
    }
}

void VoiceNav::logout()
{
    MSPLogout(); //退出登录
}

string VoiceNav::run_iat(const char* audio_file)
{
    string iat_result;
    const char* session_begin_params = "sub = iat, domain = iat, language = zh_cn, accent = mandarin, sample_rate = 16000, result_type = plain, result_encoding = utf8";
    const char* session_id = NULL;
    char rec_result[BUFFER_SIZE] = {NULL};
    char hints[HINTS_SIZE] = {NULL}; //hints为结束本次会话的原因描述，由用户自定义
    unsigned int total_len = 0;
    int aud_stat = MSP_AUDIO_SAMPLE_CONTINUE; //音频>状态
    int ep_stat =  MSP_EP_LOOKING_FOR_SPEECH; //端点>检测
    int rec_stat = MSP_REC_STATUS_SUCCESS ;   //识别状态
    int errcode =  MSP_SUCCESS ;

    FILE* f_pcm = NULL;
    char* p_pcm = NULL;
    long pcm_count = 0;
    long pcm_size = 0;
    long read_size = 0;

    if (NULL == audio_file)
        goto iat_exit;

    f_pcm = fopen(audio_file, "rb");
    if (NULL == f_pcm)
    {
        printf("\nopen [%s] failed! \n", audio_file);
        goto iat_exit;
    }

    fseek(f_pcm, 0, SEEK_END);
    pcm_size = ftell(f_pcm); //获取音频文件大小 
    fseek(f_pcm, 0, SEEK_SET);

    p_pcm = (char *)malloc(pcm_size);
    if (NULL == p_pcm)
    {
        printf("\nout of memory! \n");
        goto iat_exit;
    }

    read_size = fread((void *)p_pcm, 1, pcm_size, f_pcm); //读取音频文件内容
    if (read_size != pcm_size)
    {
        printf("\nread [%s] error!\n", audio_file);
        goto iat_exit;
    }

    printf("\n开始语音听写 ...\n");
    session_id = QISRSessionBegin(NULL, session_begin_params, &errcode); //听写不需要语法，第一个参数为NULL
    if (MSP_SUCCESS != errcode)
    {
        printf("\nQISRSessionBegin failed! error code:%d\n", errcode);
        goto iat_exit;
    }

    while (1)
    {
        unsigned int len = 10 * FRAME_LEN; // 每次写入200ms音频(16k，16bit)：1帧音频20ms，10帧=200ms。16k采样率的16位音频，一帧的大小为640Byte
        int ret = 0;

        if (pcm_size < 2 * len)
            len = pcm_size;
        if (len <= 0)
            break;

        aud_stat = MSP_AUDIO_SAMPLE_CONTINUE;
        if (0 == pcm_count)
            aud_stat = MSP_AUDIO_SAMPLE_FIRST;

        printf(">");
        ret = QISRAudioWrite(session_id, (const void *)&p_pcm[pcm_count], len, aud_stat, &ep_stat, &rec_stat);
        if (MSP_SUCCESS != ret)
        {
            printf("\nQISRAudioWrite failed! error code:%d\n", ret);
            goto iat_exit;
        }

        pcm_count += (long)len;
        pcm_size  -= (long)len;

        if (MSP_REC_STATUS_SUCCESS == rec_stat) //已经有部分听写结果
        {
            const char *rslt = QISRGetResult(session_id, &rec_stat, 0, &errcode);
            if (MSP_SUCCESS != errcode)
            {
                printf("\nQISRGetResult failed! error code: %d\n", errcode);
                goto iat_exit;
            }
            if (NULL != rslt)
            {
                unsigned int rslt_len = strlen(rslt);
                total_len += rslt_len;
                if (total_len >= BUFFER_SIZE)
                {
                    printf("\nno enough buffer for rec_result !\n");
                    goto iat_exit;
                }
                strncat(rec_result, rslt, rslt_len);
            }
        }

        if (MSP_EP_AFTER_SPEECH == ep_stat)
            break;
        usleep(20*1000); //模拟人说话时间间隙。200ms对应10帧的音频
    }
    errcode = QISRAudioWrite(session_id, NULL, 0, MSP_AUDIO_SAMPLE_LAST, &ep_stat, &rec_stat);
    if (MSP_SUCCESS != errcode)
    {
        printf("\nQISRAudioWrite failed! error code:%d \n", errcode);
        goto iat_exit;
    }

    while (MSP_REC_STATUS_COMPLETE != rec_stat)
    {
        const char *rslt = QISRGetResult(session_id, &rec_stat, 0, &errcode);
        if (MSP_SUCCESS != errcode)
        {
            printf("\nQISRGetResult failed, error code: %d\n", errcode);
            goto iat_exit;
        }
        if (NULL != rslt)
        {
            unsigned int rslt_len = strlen(rslt);
            total_len += rslt_len;
            if (total_len >= BUFFER_SIZE)
            {
                printf("\nno enough buffer for rec_result !\n");
                goto iat_exit;
            }
            strncat(rec_result, rslt, rslt_len);
        }
        usleep(15*1000); //防止频繁占用CPU
    }
    printf("=============================================================\n");
    printf("%s\n",rec_result);
    printf("=============================================================\n");
    iat_result = rec_result;

iat_exit:
    if (NULL != f_pcm)
    {
        fclose(f_pcm);
        f_pcm = NULL;
    }
    if (NULL != p_pcm)
    {
        free(p_pcm);
        p_pcm = NULL;
    }
    QISRSessionEnd(session_id, hints);
    
    return iat_result;	
}

void VoiceNav::nav_cancel(){
	ac->cancelGoal();
    cancel_flag = true;
}

void VoiceNav::send_goal(int pnum){
    goal.target_pose.header.frame_id = "map";
    goal.target_pose.header.stamp = ros::Time::now();
    
    goal.target_pose.pose.position.x = position_x[pnum];
    goal.target_pose.pose.position.y = position_y[pnum];	

    goal.target_pose.pose.orientation.z = orientation_z[pnum];
    goal.target_pose.pose.orientation.w = orientation_w[pnum];
    tts_srv.request.text = position_introduction[pnum];
    cout<<"----"<<position_name[pnum]<<"----"<<endl;
    ROS_INFO("Sending goal");
    ac->sendGoal(goal);
    send_flag = true;
}
void VoiceNav::state_detection(){
    ros::Rate rate(10);    
	while(!ac->waitForServer(ros::Duration(5.0))){
		ROS_INFO("Waiting for the move_base action server to come up");
	}
    while(ros::ok()){
        if(send_flag){
            if((ac->getState() == actionlib::SimpleClientGoalState::SUCCEEDED)){
                travel_flag = false;
                send_flag = false;
                if(tts_client.call(tts_srv)) {
                    system("aplay  ./voice_config/voice_tts/tts.wav");
                }
            }
            if(ac->getState() == actionlib::SimpleClientGoalState::PREEMPTED){
                cancel_flag =true;
                send_flag = false;
            }
        cout << ac->getState().toString() <<endl;
        ac->getResult();
        }
        
        rate.sleep();
    }
}
void VoiceNav::tarvel(){
    ros::Rate loop(10);
    for(int i=0; i<position_name.size(); i++){
        travel_flag =true;
        send_goal(i);
        while(travel_flag){
            loop.sleep();
            if(cancel_flag) goto tflag;
        }
    }
tflag:
    cancel_flag = false;
}

bool VoiceNav::voice_nav_deal(bobac2_audio::Nav::Request &req,bobac2_audio::Nav::Response &res)
{
	//actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> ac("move_base",true);
    string iat_ret;
    cancel_flag = false;
    int position_count;
    iat_ret = run_iat(req.voice_filename.data());	
    if(iat_ret.find("睡觉") != string::npos || iat_ret.find("休息") != string::npos){
        res.position = "休眠";
		tts_srv.request.text = "好的，我回去休息了，有需要随时叫我";	
		if(tts_client.call(tts_srv)) {
			system("aplay  ./voice_config/voice_tts/tts.wav");
		}
        send_goal(position_name.size()-1);
        goto flag;
    } 
    if(iat_ret.find("取消导航") != string::npos){
        res.position = "取消导航";
        nav_cancel();
        goto flag;
    }
    if(iat_ret.find("一圈") != string::npos){
        res.position = "整体游览";
        if(travel_thread.joinable()){
            cancel_flag = true;
            ROS_INFO(".....wait for new travel.....");
            travel_thread.join();
        }
        ROS_INFO("<<<<<start travel>>>>>");
        travel_thread = thread(&VoiceNav::tarvel, this);
        goto flag;
    }
    for(int i=0;i<position_name.size();i++){
        if(iat_ret.find(position_name[i]) != string::npos){
            position_count = i;
            break;
        }
    	else position_count = -1;
    }
    if(position_count == -1){
        cout<<"error position name "<<endl;
        res.position = iat_ret;
        goto flag;
    }	
    else{
        cout<<"----"<<position_name[position_count]<<"----"<<endl;
        res.position = position_name[position_count];
        send_goal(position_count);
    }
flag:	
    cout<<"----"<<res.position<<"----"<<endl;
    position_count = -1;
    return true;
}


int main(int argc,char** argv)
{
    ros::init(argc,argv,"voice_nav");
    VoiceNav  voicenav;
    
    thread detect_thread(&VoiceNav::state_detection, &voicenav);
    ros::spin();
    if(detect_thread.joinable()){
        detect_thread.join();
    }

    return 0;
}
