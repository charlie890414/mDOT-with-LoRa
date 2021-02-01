#include "mDot.h"
#include "dot_util.h"
#include "RadioEvent.h"

#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

#include <sstream>

using namespace std;

AnalogIn in(PA_5);
DigitalOut led(PC_13);

mDot *dot;
lora::ChannelPlan* plan = NULL;
Timer t;
Mutex mymutex;

static uint8_t config_app_eui[] = {0x86,0xe4,0xef,0xc7,0x10,0x4f,0x68,0x29 };
static uint8_t config_app_key[] = { 0xa3,0x46,0xb6,0xfa,0xef,0x2b,0xd3,0x3c,0x16,0xfe,0x9b,0x1d,0x8d,0x47,0xa1,0x1d};

// use the same subband as gateway that gateway can listen to you
static uint8_t config_frequency_sub_band = 1;

void PRINT(string s);
string getTime();

#if ACTIVE_EXAMPLE == OTA_EXAMPLE
void config(mDot *dot,lora::ChannelPlan *plan)
{
    uint8_t ret;
    //  reset to default config so we know what state we're in
    dot->resetConfig();
    //  set how many log info will be show
    dot->setLogLevel(mts::MTSLog::INFO_LEVEL);
    // set subband frequency the same as gateway so gateway can listen to you
    logInfo("setting frequency sub band\r\n");
    if ((ret = dot->setFrequencySubBand(config_frequency_sub_band)) != mDot::MDOT_OK) {
        logError("failed to set frequency sub band %d:%s\r\n", ret, mDot::getReturnCodeString(ret).c_str());
    }
    // lora has private network and public network here we use public network
    logInfo("setting public network mode");
    if ((ret = dot->setPublicNetwork(true)) != mDot::MDOT_OK) {
        logError("failed to public network mode");
    }
    std::vector<uint8_t> temp;

    for (int i = 0; i < 8; i++) {
        temp.push_back(config_app_eui[i]);
    }
    // set network id
    logInfo("setting app eui\r\n");
    if ((ret = dot->setNetworkId(temp)) != mDot::MDOT_OK) {
        logError("failed to set app eui %d:%s\r\n", ret, mDot::getReturnCodeString(ret).c_str());
    }
    temp.clear();
    for (int i = 0; i < 16; i++) {
        temp.push_back(config_app_key[i]);
    }
    // set network key
    logInfo("setting app key\r\n");
    if ((ret = dot->setNetworkKey(temp)) != mDot::MDOT_OK) {
        logError("failed to set app key %d:%s\r\n", ret, mDot::getReturnCodeString(ret).c_str());
    }
    // a higher spreading factor allows for longer range but lower throughput
    // in the 915 (US) frequency band, spreading factors 7 - 10 are available
    logInfo("setting TX spreading factor\r\n");
    if ((ret = dot->setTxDataRate(mDot::DR10)) != mDot::MDOT_OK) {
        logError("failed to set TX datarate %d:%s\r\n", ret, mDot::getReturnCodeString(ret).c_str());
    }
    // request receive confirmation of packets from the gateway
    logInfo("enabling ACKs\r\n");
    if ((ret = dot->setAck(1)) != mDot::MDOT_OK) {
        logError("failed to enable ACKs %d:%s\r\n", ret, mDot::getReturnCodeString(ret).c_str());
    }
    // Set Tx Power
    logInfo("enabling Tx Power\r\n");
    if ((ret = dot->setTxPower(20)) != mDot::MDOT_OK) {
        logError("failed to enable Tx Power %d:%s\r\n", ret, mDot::getReturnCodeString(ret).c_str());
    }
    // request receive confirmation of packets from the gateway
    logInfo("enabling Tx Data Rate\r\n");
    if ((ret = dot->setTxDataRate(3)) != mDot::MDOT_OK) {
        logError("failed to enable Tx Data Rate %d:%s\r\n", ret, mDot::getReturnCodeString(ret).c_str());
    }
    // save this configuration to the mDot's NVM
    logInfo("saving config\r\n");
    if (! dot->saveConfig()) {
        logError("failed to save configuration\r\n");
    }
    logInfo("joining network\r\n");
    while ((ret = dot->joinNetwork()) != mDot::MDOT_OK) {
        logError("failed to join network %d:%s\r\n", ret, mDot::getReturnCodeString(ret).c_str());
        osDelay(std::max((uint32_t)1000, (uint32_t)dot->getNextTxMs()));
    }
    return;
}
#endif

string data_str = "";

void detect(){
    int blink = 0;
    int pre = 0;
    //邊緣觸發 1:正緣 -1:負緣 0:無
    int trigger = 0;
    int count = 0;
    int ct2=0;

    int HT = 0, LT = 0;
    //狀態 0:無 1:Start 2:Alarm 3:END
    int status = 0;
    int pre_status = 0;

    t.start();
    t.reset();

    while (true) {
        float f = in.read();
        blink = (f*3.3>1) ? 1 : 0;

        //======== trigger handle =======
        trigger = blink - pre;

        if(trigger == 1){ //正緣
            LT = t.read_ms();
            t.reset();
        }else if(trigger == -1){ ///負緣
            HT = t.read_ms();
            t.reset();

            if(HT > 300) //END
                status = 3;
            else if(HT > 150 & status != 1) //Buzzer
                status = 2;
            else if(HT > 5) //Start
                status = 1;
        }

        //============ Reset =============
        if(status != 0){
            int during = t.read_ms();

            if(status == 2 & during > 1200)
                status = 0;
            if(status == 1 & during > 600)
                status = 0;
        }
        if(count>1000)
        {
            cout<<ct2<<": "<<HT<<" "<<LT<<"\n\r";
            count = 0;
            ct2++;
        }
        if(ct2>1000)
        {
			string dt = getTime();
            PRINT("working "+dt);
            ct2=0;
        }
        count++;
        //======== status change =======
        if(pre_status - status){
            //避免race condition
            mymutex.lock();

            if(status == 3)
                data_str = "END";
            if(status == 2)
                data_str = "Alarm";
            if(status == 1)
                data_str = "Boot";

            //= 復位 =
            if(status == 0){
                if(pre_status == 2)
                    data_str = "All clear";
                if(pre_status == 1)
                    data_str = "Initialized";
            }
            mymutex.unlock();
			time_t now = time(nullptr);
			string dt = getTime();
            if(data_str=="END")
            {
            	cout<<"send end msg.\n";
                PRINT("end " + dt);
			}
            else if(data_str=="Alarm")
            {
            	cout<<"send alarm msg.\n";
                PRINT("alarm " + dt);
            }
			else if(data_str=="Boot")
            {
            	cout<<"send boot msg.\n";
                PRINT("boot " + dt);
        	}
            cout << data_str << "\n\r";
            pre_status = status;

        }
        led.write(!blink);
        pre = blink;
    }
}

int main()
{
    Thread thread;
    thread.start(detect);

    while (true) {
        //避免race condition
        mymutex.lock();
        data_str = "";
        mymutex.unlock();
    }

    return 0;

}

string getTime()
{
	time_t now = time(nullptr);
	string dt;
	stringstream ss2;
	ss2<<now;
	ss2>>dt;
	return dt;
}

void PRINT(string s)
{
// object to control the debug board
    plan= new lora::ChannelPlan_AS923();

    dot = mDot::getInstance(plan);

// set network
    config(dot, plan);

     int tmp,ret;
     std::vector<uint8_t> data;
     std::string data_str;
     stringstream ss;

//      format data for sending to the gateway
     ss << s;
     ss >> data_str;
     for (std::string::iterator it = data_str.begin(); it != data_str.end(); it++)
         data.push_back((uint8_t) *it);

//      send the data to the gateway
     if ((ret = dot->send(data)) != mDot::MDOT_OK) {
         logError("failed to send\r\n", ret, mDot::getReturnCodeString(ret).c_str());
     } else {
         logInfo("successfully sent data to gateway\r\n");
     }

//      we use US but in the 868 (EU) frequency band, we need to wait until another channel is available before transmitting again
//     osDelay(std::max((uint32_t)5000, (uint32_t)dot->getNextTxMs()));
}
