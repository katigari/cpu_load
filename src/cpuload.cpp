#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "ros/ros.h"
#include "std_msgs/String.h"

#include <sstream>

#define ALARMNSET 50
#define ALARMNRES 50

#define ALARMLEVEL 75

const int NUM_CPU_STATES = 10;

enum CPUStates
{
	S_USER = 0,
	S_NICE,
	S_SYSTEM,
	S_IDLE,
	S_IOWAIT,
	S_IRQ,
	S_SOFTIRQ,
	S_STEAL,
	S_GUEST,
	S_GUEST_NICE
};

typedef struct CPUData
{
	std::string cpu;
	size_t times[NUM_CPU_STATES];
} CPUData;

void ReadStatsCPU(std::vector<CPUData> & entries);

size_t GetIdleTime(const CPUData & e);
size_t GetActiveTime(const CPUData & e);

float PrintStats(const std::vector<CPUData> & entries1, const std::vector<CPUData> & entries2);

int main(int argc, char * argv[])
{
	std::vector<CPUData> entries1;
	std::vector<CPUData> entries2;
	std::vector<CPUData> entries3;
	float cputot = -1.11;
	std::cout << cputot << "%  " << std::endl;
	///for (int i=0;i<10;i++) {
	// snapshot 1
	ReadStatsCPU(entries1);

	// 100ms pause
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// snapshot 2
	ReadStatsCPU(entries2);

	// print output
	cputot = PrintStats(entries1, entries2);

	std::cout << "final: ";
	std::cout.setf(std::ios::fixed, std::ios::floatfield);
	std::cout.width(6);
	std::cout.precision(2);
	std::cout << cputot << "%  " << std::endl;
	///}

	ros::init(argc, argv, "talker");
	ros::NodeHandle n;
	ros::Publisher chatter_pub = n.advertise<std_msgs::String>("cpuload", 1000);
ros::Publisher cpualarm_pub = n.advertise<std_msgs::String>("cpuload2", 1000);
	ros::Rate loop_rate(10);

	int count1,count2 = 0;
  while (ros::ok())
  {
	// snapshot 1
	ReadStatsCPU(entries1);

	// 100ms pause
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// snapshot 2
	ReadStatsCPU(entries2);

	// print output
	cputot = PrintStats(entries1, entries2);

    
	/**
     * This is a message object. You stuff it with data, and then publish it.
     */
    std_msgs::String msg,msg2;

    std::stringstream ss,ss2;

    ss << "CPU Usage " << cputot;
	//ss2 << "CPU ALARM " << count;

    msg.data = ss.str();
	//msg2.data = ss2.str();

    ROS_INFO("%s", msg.data.c_str());
	
    /**
     * The publish() function is how you send messages. The parameter
     * is the message object. The type of this object must agree with the type
     * given as a template parameter to the advertise<>() call, as was done
     * in the constructor above.
     */
    chatter_pub.publish(msg);
	if (cputot > ALARMLEVEL)
	{
    	count1++;
		
		if (count1 > ALARMNSET) 
		{
			ss2 << "CPU ALARM " << count1;
			msg2.data = ss2.str();
			ROS_INFO("%s", msg2.data.c_str());			
			cpualarm_pub.publish(msg2);		
			count2 = 0;
		}
	}
	else if (cputot < ALARMLEVEL && count1 > ALARMNSET)
	{
		count2++;
	}
	else if (count1 > ALARMNSET && count2 > ALARMNRES)
	{
		count1 = 0;
		count2 = 0;
		std::cout << "ALARM reset "<< std::endl; 

	}
	
    ros::spinOnce();

    loop_rate.sleep();

	entries1 = entries3;
	entries2 = entries3;
  }


	return 0;
}

void ReadStatsCPU(std::vector<CPUData> & entries)
{
	std::ifstream fileStat("/proc/stat");

	std::string line;

	const std::string STR_CPU("cpu");
	const std::size_t LEN_STR_CPU = STR_CPU.size();
	const std::string STR_TOT("tot");

	while(std::getline(fileStat, line))
	{
		// cpu stats line found
		if(!line.compare(0, LEN_STR_CPU, STR_CPU))
		{
			std::istringstream ss(line);

			// store entry
			entries.emplace_back(CPUData());
			CPUData & entry = entries.back();

			// read cpu label
			ss >> entry.cpu;

			// remove "cpu" from the label when it's a processor number
			if(entry.cpu.size() > LEN_STR_CPU)
				entry.cpu.erase(0, LEN_STR_CPU);
			// replace "cpu" with "tot" when it's total values
			else
				entry.cpu = STR_TOT;

			// read times
			for(int i = 0; i < NUM_CPU_STATES; ++i)
				ss >> entry.times[i];
		}
	}
}

size_t GetIdleTime(const CPUData & e)
{
	return	e.times[S_IDLE] + 
			e.times[S_IOWAIT];
}

size_t GetActiveTime(const CPUData & e)
{
	return	e.times[S_USER] +
			e.times[S_NICE] +
			e.times[S_SYSTEM] +
			e.times[S_IRQ] +
			e.times[S_SOFTIRQ] +
			e.times[S_STEAL] +
			e.times[S_GUEST] +
			e.times[S_GUEST_NICE];
}

//void PrintStats(const std::vector<CPUData> & entries1, const std::vector<CPUData> & entries2)
float PrintStats(const std::vector<CPUData> & entries1, const std::vector<CPUData> & entries2)
{
	const size_t NUM_ENTRIES = entries1.size();
	float total[NUM_CPU_STATES];
	float out=-1.11;
	////for(size_t i = 0; i < 1; ++i)//NUM_ENTRIES; ++i)
	////{
		
		const CPUData & e1 = entries1[0];
		const CPUData & e2 = entries2[0];

		//std::cout.width(3);
		//std::cout << e1.cpu << "] ";

		const float ACTIVE_TIME	= static_cast<float>(GetActiveTime(e2) - GetActiveTime(e1));
		const float IDLE_TIME	= static_cast<float>(GetIdleTime(e2) - GetIdleTime(e1));
		const float TOTAL_TIME	= ACTIVE_TIME + IDLE_TIME;

		//std::cout << "active: ";
		//std::cout.setf(std::ios::fixed, std::ios::floatfield);
		//std::cout.width(6);
		//std::cout.precision(2);
		//std::cout << "  " << (100.f * ACTIVE_TIME / TOTAL_TIME) << "%";
		out = 100.f * ACTIVE_TIME / TOTAL_TIME;
		//std::cout << " - idle: ";
		//std::cout.setf(std::ios::fixed, std::ios::floatfield);
		//std::cout.width(6);
		//std::cout.precision(2);
		//std::cout << (100.f * IDLE_TIME / TOTAL_TIME) << "%" << std::endl;
	////}
	std::cout << "  " << out << "%";
	return out;
}
