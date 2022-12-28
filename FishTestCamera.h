#pragma once

#include <iostream>
#include <opencv2/opencv.hpp>
#include <pigpio.h>

#include <filesystem>
#include <vector>
#include <string>

using namespace std;

//Length of interval between LEDs turning on, and off


//State of class, either taking a picture or running a video
enum
{
	CAMERA_OFF,
	CAMERA_PICTURE,
	CAMERA_VIDEO
};

class FishTestCamera
{
public:
	FishTestCamera(int flash_leds_pin, int video_led_pin, int success_led_pin, string file_path = "./data/", cv::Size cam_size = cv::Size(960, 720));
	~FishTestCamera();
	
	/**
	 ** @brief Run in while loop, manages class runtime
	 ***/
	void run();
	
private:
	/******	MEMBER VARIABLES ******/
	
	//OpenCV Video stream
	cv::VideoCapture _cam;
	
	//OpenCV mat for video or pic
	cv::Mat _im;
	
	//Pins for various LEDs
	int _flash_leds_pin;
	int _video_led_pin;
	int _success_led_pin;
	
	//LED needs to be strobed
	bool _led_state;
	
	//Timer for LED strobing
	double _led_timer;
	
	//Information for file names and path of pictures and video
	int _picture_count;
	int _video_count;
	string _file_path_video;
	string _file_path_picture;
	
	//Timer between pictures - When button gets pressed, two pictures need to be taken
	double _picture_timer;
	
	//Two pictures need to be taken, keep track of which picture has been taken so far
	int _picture_state;
	
	//State of camera, refer to enums for states
	int _camera_state;
	
	
	/******	METHODS ******/	
	void _record_video();
	void _record_pictures();
};