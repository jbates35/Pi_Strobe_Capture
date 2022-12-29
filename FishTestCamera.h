#pragma once

#include <thread>
#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <sstream>
#include <cstdlib>
#include <fstream>

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>

#include "cvui.h"

#include <pigpio.h>

using namespace std;

#define CANVAS_NAME		"Preview Camera"

#define EXPOSURE_DEFAULT	100		//Default exposure of camera
#define EXPOSURE_MIN		20		//Min exposure of camera
#define EXPOSURE_MAX		250		//Max exposure of camera

#define BRIGHTNESS_DEFAULT	50		//Default brightness of camera
#define BRIGHTNESS_MIN		0		//Min brightness of camera
#define BRIGHTNESS_MAX		100		//Max brightness of camera

#define CONTRAST_DEFAULT	50		//Default contrast of camera
#define CONTRAST_MIN		0		//Min contrast of camera
#define CONTRAST_MAX		100		//Max contrast of camera

#define SATURATION_DEFAULT	50		//Default contrast of camera
#define SATURATION_MIN		0		//Min saturation of camera
#define SATURATION_MAX		100		//Max saturation of camera

#define FRAME_PERIOD_DEFAULT 30		//Default frame rate
#define FRAME_PERIOD_MIN	5		//Minimum framerate for video
#define FRAME_PERIOD_MAX	60		//Max framerate for video

//State of class, either taking a picture or running a video
enum
{
	CAMERA_OFF,
	CAMERA_PICTURE,
	CAMERA_VIDEO
};

enum
{
	VIDEO_RECORD,
	VIDEO_DONE
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
	FishTestCamera(int flash_leds_pin, int video_led_pin, int success_led_pin, int button_1_pin, int button_2_pin, cv::Size cam_size = cv::Size(640, 480));
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
	 ** @brief Set button 1 to on
	 ***/
	void set_button_1();
	
	/**
	 ** @brief Set button 2 to on
	 ***/
	void set_button_2();
	
	/**
	 ** @brief Getter for waitKey variable
	 ***/
	char esc_key()
	{
		return _esc_key;
	}
	
	/**
	 ** @brief Getter for quit button
	 ***/
	char esc_button()
	{
		return _esc_button;
	}	
	
	
private:
	/******	MEMBER VARIABLES ******/	
	//OpenCV Video stream
	cv::VideoCapture _camera;
	cv::Size _camera_size;
		
	//OpenCV mat for video or pic
	cv::Mat _image;
	
	//OpenCV video object for recording
	cv::VideoWriter _video;
	
	//CVUI parameters
	bool _show_cvui;
	
	//Camera parameters
	int _exposure, _exposure_prev;
	int _brightness, _brightness_prev;
	int _contrast, _contrast_prev;
	int _saturation, _saturation_prev;
	
	//For assigning waitKey to
	char _esc_key;
	
	//For assigning quit button to
	char _esc_button;
	
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
	
	//Timer for how long video lasts
	double _video_timer;
	
	//Need video state machine for whether it's starting, recording or if it's done
	int _video_state;
	
	//Desired framerate for videos, changed by slider
	int _video_frame_period;
	
	//Frame count and timer
	int _frame_count;
	double _frame_timer;
	
	//State of camera, refer to enums for states
	int _camera_state;
	
	//Turns on if picture or video has been saved correctly, and then starts function in run
	bool _success_signal;
	
	
	/******	METHODS ******/	
	//Need preview camera, but camera isn't recording or taking picture in this state
	void _camera_off();
	
	//Records video, once flag has been turned off, save file
	void _record_video();
	
	//Turn flash on, take picture, turn flash off, take picture, save files
	void _record_pictures();
	
	//Makes sure camera is initialized/turned on, sets width/height
	void _init_cam();
	
	//Adds trackbars for certain parameters to be adjusted
	void _add_trackbars();
	
	//Updates camera settings based on trackbar input
	void _update_camera_settings();
	
	//Fills the time buffer with current time
	string _get_time();
	
	//Write to a file holding log info in the specified folder
	void _write_file(string input, string path);	
	
	//If successful, flash the green LEDs
	void _show_success();
	
	//Creates thread for _show_success
	static void _show_success_thread(FishTestCamera* ptr);
	
	//Function invoked to actually start the thread
	void _call_show_success();
};