#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <sstream>
#include <cstdlib>
#include <fstream>

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>

#include <pigpio.h>

using namespace std;
namespace fs = std::filesystem;

#define ISR_TIMEOUT		100000 // microseconds

#define VID_BUTTON_INTERVAL 1 // Amount of time to hold video toggle button

#define STROBE_INT 0.05  //Length of interval between LEDs turning on, and off (in s)

//State of class, either taking a picture or running a video
enum
{
	CAMERA_OFF,
	CAMERA_PICTURE,
	CAMERA_VIDEO_ON,
	CAMERA_VIDEO_DONE
};

class FishTestCamera
{
public:
	/**
	 ** @brief Initializes essentially the entire camera->fish eye testing system
	 **
	 ** @param flash_leds_pin Output pin for lights around camera
	 ** @param video_led_pin Output pin for green video recording light
	 ** @param success_led_pin Output pin for signalling that video or picture was taken successfully
	 ** @param button_1_pin Input pin for button 1
	 ** @param button_2_pin Input pin for button 2
	 **	@param cam_size 
	 ***/
	FishTestCamera(int flash_leds_pin, int video_led_pin, int success_led_pin, int button_1_pin, int button_2_pin, cv::Size cam_size = cv::Size(960, 720));
	~FishTestCamera();
	
	/**
	 ** @brief Initializes GPIO signals, filesystem, and camera
	 ** 
	 **	@return 0 if success, -1 if failure
	 ***/
	int init();
	
	/**
	 ** @brief Run in while loop, manages class runtime
	 ***/
	void run();
	
	/**
	 ** Set button 1 to on
	 ***/
	void set_button_1();
	
	/**
	 ** Set button 2 to on
	 ***/
	void set_button_2();
	
	
private:
	/******	MEMBER VARIABLES ******/	
	//OpenCV Video stream
	cv::VideoCapture _camera;
	cv::Size _camera_size;
	
	//OpenCV mat for video or pic
	cv::Mat _image;
	
	//Pins for various LEDs
	int _flash_leds_pin;
	int _video_led_pin;
	int _success_led_pin;
	
	//Pins for buttons
	int _button_1_pin;
	int _button_2_pin;

	//Flags to telling program that buttons been pressed, run ISR
	int _button_1_pressed;
	int _button_2_pressed;
	
	//Extra debounce for video
	double _button_2_timer;
		
	//LED needs to be strobed
	bool _led_state;
	
	//Timer for LED strobing
	double _led_timer;
	
	//Information for file names and path of pictures and video
	int _picture_count;
	int _video_count;
	string _file_path_base;
	string _file_path_video;
	string _file_path_picture;
		
	//Information about picture and/or video
	stringstream _file_info_ss;
		
	//Timer between pictures - When button gets pressed, two pictures need to be taken
	double _picture_timer;
	
	//Two pictures need to be taken, keep track of which picture has been taken so far
	int _picture_state;
	
	//State of camera, refer to enums for states
	int _camera_state;
	
	//Turns on if picture or video has been saved correctly, and then starts function in run
	bool _success_signal;
	
	
	/******	METHODS ******/	
	//Need preview camera, but camera isn't recording or taking picture in this state
	void _camera_off();
	
	//Records video, once flag has been turned off, save file
	void _record_video();
	
	//When camera is done, save video stream to file
	void _save_video();
	
	//Turn flash on, take picture, turn flash off, take picture, save files
	void _record_pictures();
	
	//Makes sure camera is initialized/turned on, sets width/height
	void _init_cam();
	
	//Fills the time buffer with current time
	string _get_time();
	
	//Write to a file holding log info in the specified folder
	void _write_file(string input, string path);	
};