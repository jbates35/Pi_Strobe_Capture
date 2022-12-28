#include "FishTestCamera.h"

FishTestCamera::FishTestCamera(int flash_leds_pin, int video_led_pin, int success_led_pin, string file_path, cv::Size cam_size)
{
	//Set up video stream
	_cam.open(0);
	
	//Set video stream width/height
	_cam.set(cv::CAP_PROP_FRAME_WIDTH, cam_size.width);
	_cam.set(cv::CAP_PROP_FRAME_HEIGHT, cam_size.height);
	
	//Path for picture files and create directory
	_file_path_video = file_path + "video";
	
	//Initialize pic count
	
	//Path for video files, create directory
	_file_path_picture = file_path + "pictures";
	
	//Initialize video count
	
	//Initialize other member variables
	
	//Intialize camera state
	
	//Store pins for LEDs, set LEDs
	
}

FishTestCamera::~FishTestCamera()
{
}


void FishTestCamera::run()
{
}


void FishTestCamera::_record_video()
{
}


void FishTestCamera::_record_pictures()
{
}
