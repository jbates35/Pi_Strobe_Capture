#include "FishTestCamera.h"

FishTestCamera::FishTestCamera(int flash_leds_pin, int video_led_pin, int success_led_pin, int button_1_pin, int button_2_pin, cv::Size cam_size)
{
	//Store cam size into private member
	_camera_size = cam_size;
	
	//Store pins for LEDs
	_flash_leds_pin = flash_leds_pin;
	_video_led_pin = video_led_pin;
	_success_led_pin = success_led_pin;		
	
	//Store pins for Buttons
	_button_1_pin = button_1_pin;
	_button_2_pin = button_2_pin;	
	
	//Initialize trackbars for cvui
	cvui::init(CANVAS_NAME);
}

FishTestCamera::~FishTestCamera()
{
	//Terminate GPIO
	gpioTerminate();
	
	//End camera, release any video files
	if (_video.isOpened())
	{
		_video.release();
	}
	
	if (_camera.isOpened())
	{
		_camera.release();
	}
	
	//Close any remaining windows
	cv::destroyAllWindows();
}

//Initialize dynamic elements
int FishTestCamera::init()
{	
	//Initialize GPIO and return as failure if unsuccessful
	if (gpioInitialise() < 0) 
	{
		return -1;
	}
	
	//Open video and set height/width
	_init_cam();
	
	//Get current time for folder
	string current_time = _get_time();
	
	//Create base file path to build other folders from
	_file_path_base = "./data/" + current_time + "/";

	//Path for video and picture files and create directories
	_file_path_video = _file_path_base + "video/";
	_file_path_picture = _file_path_base + "pictures/";
	
	//Make command for VIDEO directory and then convert it to char
	string file_command = "mkdir -p " + _file_path_video;
		
	//Make directory, return error if not possible
	if (system(file_command.c_str()) == -1)
	{
		std::cout << "Failed to make video directory, exiting program\n";
		return -1;
	}
	
	//Now make command for PICTURE directory and convert it to char
	file_command = "mkdir -p " + _file_path_picture;
	
	//Make directory, return error if not possible
	if (system(file_command.c_str()) == -1)
	{
		std::cout << "Failed to make picture directory, exiting program\n";
		return -1;
	}
		
	//Now change the two directories to chmod +777	
	if (system("chmod -R 777 ./") == -1)
	{
		std::cout << "Failed to change permissions, exiting program\n";
		return -1;			
	}
	
	//Set camera to manual exposure
	system("v4l2-ctl --device /dev/video0 -c auto_exposure=1");	
	
	//Initialize pic and video count
	_picture_count = 0;
	_video_count = 0;
		
	//Intialize camera state
	_camera_state = CAMERA_OFF;
	_picture_state = 0;
	
	//Default framerate for video
	_video_frame_period = FRAME_PERIOD_DEFAULT;

	//Initialize camera and cvui parameters
	_exposure = EXPOSURE_DEFAULT;
	_brightness = BRIGHTNESS_DEFAULT;
	_contrast = CONTRAST_DEFAULT;
	_saturation = SATURATION_DEFAULT;
	_show_cvui = false;
	
	//Initialize LED state
	_led_state = false;
		
	//Initialize quit and waitkeys
	_esc_button = '\0';
	_esc_key = '\0';
	
	//LED Pin setup
	gpioSetMode(_flash_leds_pin, PI_OUTPUT);	
	gpioSetMode(_video_led_pin, PI_OUTPUT);	
	gpioSetMode(_success_led_pin, PI_OUTPUT);
	
	//Initialize LEDs
	gpioWrite(_flash_leds_pin, 0);
	gpioWrite(_video_led_pin, 0);
	gpioWrite(_success_led_pin, 0);
	
	return 0;
}

//Continuous loop - takes care of organizing camera state machine and running script
void FishTestCamera::run()
{
	//State machine for camera
	switch (_camera_state)
	{
	case CAMERA_OFF:
		_camera_off();
		break;
		
	case CAMERA_PICTURE:
		_record_pictures();
		break;
		
	case CAMERA_VIDEO:
		_record_video();
		break;
		
	default:
		break;
	}	
}

//ISR for button 1 - turn picture mode on
void FishTestCamera::set_button_1()
{	
	//If camera is off, turn camera state to picture mode
	if (_camera_state == CAMERA_OFF)
	{				
		//Reset picture state
		_picture_state = 0;
		
		//Turn camera state to picture mode
		_camera_state = CAMERA_PICTURE;
	}
}

//ISR for button 2 - starts timer for run thread to see if button is pressed for 2 seconds
void FishTestCamera::set_button_2()
{	
	//If camera state machine is video, can then invoke saving file
	if (_camera_state == CAMERA_VIDEO)
	{
		std::cout << "Button 2 registered, turning video off\n";
				
		//Change video state to tell script to end file, save it, etc.
		_video_state = VIDEO_DONE;
	}
			
	//If camera state machine is off, can turn video mode on
	else if (_camera_state == CAMERA_OFF)
	{
		std::cout << "Button 2 registered, turning video on\n";
				
		//Set camera state
		_camera_state = CAMERA_VIDEO;
				
		//Set video state to 'record'
		_video_state = VIDEO_RECORD;
	}	
}

//Turn flash on, take picture, turn flash off, take picture, save files
void FishTestCamera::_camera_off()
{
	//Make sure camera is running
	if (_camera.isOpened() == false)
	{
		//Initiailize camera if need be
		_init_cam();
		
		if (_camera.isOpened() == false)
		{
			return;
		}
	}
	
	//Make sure cam can be run
	gpioSleep(PI_TIME_RELATIVE, 0, 5000);
	
	//Load camera to frame
	_camera.read(_image);
	
	//Adds trackbars for camera settings
	_add_trackbars();
	
	//Show camera for preview
	cv::imshow(CANVAS_NAME, _image);
	_esc_key = cv::waitKey(10);
}

//Records video, once flag has been turned off, save file
void FishTestCamera::_record_video()
{
	//Clear file stream for logging
	_file_info_ss.str("");
	
	//Reset timer
	_video_timer = cv::getTickCount();
	
	//Make sure camera is running
	if (_camera.isOpened() == false)
	{
		_init_cam();
		
		//Still no dice? Record log
		if (_camera.isOpened() == false)
		{
			_file_info_ss << "Couldn't open video, must return";
			
			//Write to log file
			_write_file(_file_info_ss.str(), _file_path_video + to_string(_video_count) + "_log.txt");
			
			return;
		}
	}
		
	//Initialize video
	if (_video_state == 0)
	{
		//Turn blue LEDs on
		gpioWrite(_video_led_pin, 1);		
		
		//Reset frame counter
		_frame_count = 0;
		
		//Load camera to frame
		_camera.read(_image);
		
		//Store parameters for video
		string file_name = _file_path_video + to_string(_video_count) + ".avi";
		int codec = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
		double fps = 30.0;
		bool is_color = (_image.type() == CV_8UC3);
		
		//Open video stream and record
		_video.open(file_name, codec, fps, _image.size(), is_color);
		
		//Ensure video file is open
		if (!_video.isOpened()) 
		{
			//Write error msg
			_file_info_ss << "Could not open the output video file for write\n";
			
			//Turn blue LEDs off
			gpioWrite(_video_led_pin, 0);
			
			//Write to log file
			_write_file(_file_info_ss.str(), _file_path_video + to_string(_video_count) + "_log.txt");
			
			return;
		}
		
		//Log start of video
		std::cout << "Writing video " << _video_count << ".avi...\n";		
	}
		
	//Record video and show frames
	while (_video_state == VIDEO_RECORD && _esc_button != 'q' && _esc_key != 'q')
	{
		//Initialize timer for grabbing frame time
		_frame_timer = cv::getTickCount();
						
		//Return if blank frame grabbed
		if (!_camera.read(_image))
		{
			_file_info_ss << "WARNING: Grabbed blank frame, ending video...\n";
			
			break;
		}
		
		//Capture frame
		_video.write(_image);	
		
		//Toggle led state and strobe lights
		_led_state = !_led_state;
		gpioWrite(_flash_leds_pin, _led_state);
		
		//Adds trackbars for camera settings
		_add_trackbars();
			
		//Draw red rectangle around frame, also say recording
		cv::rectangle(_image, cv::Point(1, 1), cv::Point(_image.size().width - 1, _image.size().height - 1), cv::Scalar(0, 0, 255), 3);
		cv::putText(_image, "Recording", cv::Point(20, 20), cv::FONT_HERSHEY_DUPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
		
		//Show live while recording
		cv::imshow(CANVAS_NAME, _image);
		
		//Delay so LED flash can be fully on or off in the shot (not in transition)
		_esc_key = cv::waitKey(_video_frame_period);		
		
		//Increment frame count, get time, write to log
		_frame_count++;
		_file_info_ss << "Frame " << _frame_count << " time: " << round(1000*(cv::getTickCount() - _frame_timer) / cv::getTickFrequency()) << "ms \n";		
	}
		
	//Save video file to file
	_video.release();
	_camera.release();
	
	//Write remaining file info
	_file_info_ss << "Exposure of camera: " << _exposure << "\n";
	_file_info_ss << "Brightness of camera: " << _brightness << "\n";
	_file_info_ss << "Contrast of camera: " << _contrast << "\n";
	_file_info_ss << "Saturation of camera: " << _saturation << "\n";
	_file_info_ss << "Video frame period: " << _video_frame_period << "\n";
	_file_info_ss << "File " << _video_count << ".avi successfully saved to " << _file_path_video << "\n";
	_file_info_ss << "Length of video: " << (cv::getTickCount() - _video_timer) / cv::getTickFrequency() << "s\n";
	_file_info_ss << "Date and time of video record: " << _get_time() << "\n\n";

		
	//Write to log file
	_write_file(_file_info_ss.str(), _file_path_video + to_string(_video_count) + "_log.txt");
	
	//Video is done recording, so can turn blue LEDs off
	gpioWrite(_video_led_pin, 0);	
		
	//Make sure flash LEDs are off
	_led_state = 0;
	gpioWrite(_flash_leds_pin, _led_state);
	
	//Turn success LEDs on (calls thread)
	_call_show_success();
	
	//Reset camera state
	_camera_state = CAMERA_OFF;	
	
	//Reset video state
	_video_state = VIDEO_RECORD;
		
	//Increment video count
	_video_count++;		
}

//Turn flash on, take picture, turn flash off, take picture, save files
void FishTestCamera::_record_pictures()
{
	//Record current directory for creating and then saving pics to
	string curr_file_path = _file_path_picture + to_string(_picture_count) + "/";
	
	//Take picture with flash off and save
	if (_picture_state == 0)
	{
		//Release camera if need be
		if (_camera.isOpened() == true)
		{
			_camera.release();	
		}
		
		//Open camera
		_init_cam();
		
		//Clear stringstream for taking in file data
		_file_info_ss.str("");
		
		//Make directory
		string make_dir_command = "mkdir -p " + curr_file_path;
		
		//Make C version of string so unix can use
		char *make_dir_command_char;		
		make_dir_command_char = (char*) malloc(make_dir_command.size() + 1);
	
		//Dump C++ string into C string
		for (int char_ind = 0; char_ind < make_dir_command.size(); char_ind++)
		{
			make_dir_command_char[char_ind] = make_dir_command[char_ind];
		}
		make_dir_command_char[make_dir_command.size()] = '\0';
		
		//Make directory
		system(make_dir_command_char);
		
		//Sleep 1ms
		gpioSleep(PI_TIME_RELATIVE, 0, 1000);
		
		//Take picture and save
		if (_camera.read(_image)) 
		{			
			cv::imwrite(curr_file_path + to_string(_picture_count) + "_flash_off.jpg", _image);		
			
			_file_info_ss << "Image: " << _picture_count << "_flash_off.jpg successfully saved to " << curr_file_path << " at " << _get_time() << "\n";		
		}
		else
		{
			_file_info_ss << "Error: Image: " << _picture_count << "_flash_off.jpg could not be saved to " << curr_file_path << " at " << _get_time() << "\n";
		}
		
		//Advance state machine
		_picture_state++;
		
		//Turn LEDs on
		gpioWrite(_flash_leds_pin, 1);
		
		//Reset picture timer
		_picture_timer = cv::getTickCount();
		
	}
	
	//Take picture with flash on and save
	if (_picture_state == 1)
	{				
		//Sleep 1ms
		gpioSleep(PI_TIME_RELATIVE, 0, 1000);
		
		//Add time for buffering
		for (int capture_count = 0; capture_count < 1; capture_count++)
		{
			_camera.read(_image);		
			
			//Sleep 1ms
			gpioSleep(PI_TIME_RELATIVE, 0, 1000);
		}
		
		//Take picture and save
		if (_camera.read(_image)) 
		{			
			cv::imwrite(curr_file_path + to_string(_picture_count) + "_flash_on.jpg", _image);
			
			_file_info_ss << "Image: " << _picture_count << "_flash_on.jpg successfully saved to " << curr_file_path << " at " << _get_time() << "\n";		
		}
		else
		{
			_file_info_ss << "Error: Image: " << _picture_count << "_flash_on.jpg could not be saved to " << curr_file_path << " at " << _get_time() << "\n";
		}
		
		//Write time it between shots
		_file_info_ss << "Time distance between camera shots: " <<  (cv::getTickCount() - _picture_timer) / cv::getTickFrequency() << "\n\n";
		
		//Write other camera parameters
		_file_info_ss << "Exposure of camera: " << _exposure << "\n";
		_file_info_ss << "Brightness of camera: " << _brightness << "\n";
		_file_info_ss << "Contrast of camera: " << _contrast << "\n";
		_file_info_ss << "Saturation of camera: " << _saturation << "\n";
		
		//Write it to display, and save it to file as well
		std::cout << _file_info_ss.str();
		_write_file(_file_info_ss.str(), curr_file_path + to_string(_picture_count) + "_log.txt");
		
		//Change permission on all to 777
		system("chmod -R 777 ./");
			
		//Reset state machines so it can exit picture mode
		_picture_state++;
		_camera_state = CAMERA_OFF;
		
		//Increment video count
		_picture_count++;
		
		//Turn LEDs off
		gpioWrite(_flash_leds_pin, 0);
				
		//Turn success LEDs on (calls thread)
		_call_show_success();
		
		//Release camera
		_camera.release();	
		
		//Re-initializes camera
		_init_cam();		
	}
}

//Makes sure camera is initialized/turned on, sets width/height
void FishTestCamera::_init_cam()
{
	//Set up video stream
	if (_camera.isOpened() == false)
	{
		_camera.open(0);	
		gpioSleep(PI_TIME_RELATIVE, 0, 1000);
	}
	
	//Set video stream width/height
	_camera.set(cv::CAP_PROP_FRAME_WIDTH, _camera_size.width);
	_camera.set(cv::CAP_PROP_FRAME_HEIGHT, _camera_size.height);
	
}

//Adds trackbars for certain parameters to be adjusted
void FishTestCamera::_add_trackbars()
{
	//Initial variables for trackbar window
	cv::Point update_window_pos;
	int height;	
	string visibility_button_str;
	
	//Adjust update window whether it should be hidden or not
	if (_show_cvui == true) 
	{
		height = _image.size().height;
		visibility_button_str = "Hide settings";
	}
	else
	{
		height = 50;
		visibility_button_str = "Show settings";
	}
	
	//Window setting x
	update_window_pos.x = _image.size().width - 200;
	
	//Initiate window
	cvui::window(_image, update_window_pos.x, update_window_pos.y, 200, height, "Settings and functions");
	
	//Show/hide button position
	update_window_pos.y += 25;
	
	//Show/hide button implementation
	if (cvui::button(_image, update_window_pos.x+50, update_window_pos.y, 100, 25, visibility_button_str)) 
	{
		//Change whether trackbar is shown or not
		_show_cvui = !_show_cvui;
		
		//Change height now so that we don't have the other buttons accidentally selected
		height = _image.size().height;
	}	
	
	//If "show" is selected, write rest of cvui
	if (_show_cvui) 
	{
		//Move position of update settings position down
		update_window_pos.y += 45;
	
		//Change exposure of camera
		cvui::trackbar(_image, update_window_pos.x + 10, update_window_pos.y, 180, &_exposure, EXPOSURE_MIN, EXPOSURE_MAX);
		cvui::text(_image, update_window_pos.x + 65, update_window_pos.y, "Exposure");
		
		//Move position of update settings position down
		update_window_pos.y += 70;
		
		//Change exposure of camera
		cvui::trackbar(_image, update_window_pos.x + 10, update_window_pos.y, 180, &_brightness, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
		cvui::text(_image, update_window_pos.x + 65, update_window_pos.y, "Brightness");
		
		//Move position of update settings position down
		update_window_pos.y += 70;
		
		//Change exposure of camera
		cvui::trackbar(_image, update_window_pos.x + 10, update_window_pos.y, 180, &_contrast, CONTRAST_MIN, CONTRAST_MAX);
		cvui::text(_image, update_window_pos.x + 65, update_window_pos.y, "Contrast");
		
		//Move position of update settings position down
		update_window_pos.y += 70;
		
		//Change exposure of camera
		cvui::trackbar(_image, update_window_pos.x + 10, update_window_pos.y, 180, &_saturation, SATURATION_MIN, SATURATION_MAX);
		cvui::text(_image, update_window_pos.x + 65, update_window_pos.y, "Saturation");
		
		//Move position of update settings position down
		update_window_pos.y += 70;
		
		//Change framerate of video
		cvui::trackbar(_image, update_window_pos.x + 10, update_window_pos.y, 180, &_video_frame_period, FRAME_PERIOD_MIN, FRAME_PERIOD_MAX);
		cvui::text(_image, update_window_pos.x + 60, update_window_pos.y, "Frame period");		
		
		//Put in buttons for picture and video
		update_window_pos.y = height - 50;
	
		//Take picture button
		if (cvui::button(_image, update_window_pos.x, update_window_pos.y, 100, 25, "Picture")) 
		{
			//Essentially, this is like pressing button 1
			set_button_1();
		}	
		
		//Take picture button
		if (cvui::button(_image, update_window_pos.x + 100, update_window_pos.y, 100, 25, "Video")) 
		{
			//Essentially, this is like pressing button 2
			set_button_2();
		}

		//Put in buttons for picture and video
		update_window_pos.y = height - 75;
		
		//Default values
		if (cvui::button(_image, update_window_pos.x+100, update_window_pos.y, 100, 25, "Default Values")) 
		{
			//Default framerate for video
			_video_frame_period = FRAME_PERIOD_DEFAULT;

			//Initialize camera and cvui parameters
			_exposure = EXPOSURE_DEFAULT;
			_brightness = BRIGHTNESS_DEFAULT;
			_contrast = CONTRAST_DEFAULT;
			_saturation = SATURATION_DEFAULT;
		}		
	}	
	
	//Add quit window
	if (cvui::button(_image, _image.size().width - 75, _image.size().height - 25, 75, 25, "Quit"))
	{
		_esc_button = 'q';
	}
	
	//Update all trackbars
	cvui::update();
	
	//Update camera settings afterwards
	_update_camera_settings();
}

//Updates camera settings based on trackbar input
void FishTestCamera::_update_camera_settings()
{
	//Only do this if exposure has been changed
	if (_exposure_prev != _exposure) 
	{
		_exposure_prev = _exposure;

		//Update exposure
		string exposure_command = "v4l2-ctl --device /dev/video0 -c exposure_time_absolute=" + to_string(_exposure);
		system(exposure_command.c_str());		
	}
	
	//Only do this if brightness has been changed
	if (_brightness_prev != _brightness) 
	{
		_brightness_prev = _brightness;

		//Update exposure
		string brightness_command = "v4l2-ctl --device /dev/video0 -c brightness=" + to_string(_brightness);
		system(brightness_command.c_str());		
	}
	
	//Only do this if contrast has been changed
	if (_contrast_prev != _contrast) 
	{
		_contrast_prev = _contrast;

		//Update exposure
		string contrast_command = "v4l2-ctl --device /dev/video0 -c contrast=" + to_string(_contrast);
		system(contrast_command.c_str());		
	}
	
	//Only do this if exposure has been changed
	if (_saturation_prev != _saturation) 
	{
		_saturation_prev = _saturation;

		//Update exposure
		string saturation_command = "v4l2-ctl --device /dev/video0 -c saturation=" + to_string(_saturation);
		system(saturation_command.c_str());		
	}
}

//Get date and time in yyyy_mm_dd_hxxmxxsxx
string FishTestCamera::_get_time()
{		
	//Create unique timestamp for folder
	stringstream timestamp;
	
	//First, create struct which contains time values
	time_t now = time(0);
	tm *ltm = localtime(&now);
	
	//Store stringstream with numbers	
	timestamp << 1900 + ltm->tm_year << "_";
	
	//Zero-pad month
	if ((1 + ltm->tm_mon) < 10) 
	{
		timestamp << "0";
	}
	
	timestamp << 1 + ltm->tm_mon << "_";
	
	//Zero-pad day
	if ((1 + ltm->tm_mday) < 10) 
	{
		timestamp << "0";
	}
	
	timestamp << ltm->tm_mday << "_";
	
	//Zero-pad hours
	if (ltm->tm_hour < 10) 
	{
		timestamp << "0";
	}
	
	timestamp << ltm->tm_hour << "h";
	
	//Zero-pad minutes
	if (ltm->tm_min < 10) 
	{
		timestamp << "0";
	}
	
	timestamp << ltm->tm_min << "m";
	
	//Zero-pad seconds
	if (ltm->tm_sec < 10) 
	{
		timestamp << "0";
	}
	
	timestamp << ltm->tm_sec << "s";
	
	//Return string version of ss
	return timestamp.str();
}

//Write to a file holding log info in the specified folder
void FishTestCamera::_write_file(string input, string path)
{
	//Create log file
	std::ofstream out_file(path);
	
	//Write to file
	out_file << input;
	
	//Close file
	out_file.close();
}

//Flash green LED to show that video or picture has been successful
void FishTestCamera::_show_success()
{
	//Flash thrice
	gpioWrite(_success_led_pin, 1);
	gpioSleep(PI_TIME_RELATIVE, 0, 500000);
	gpioWrite(_success_led_pin, 0);
	gpioSleep(PI_TIME_RELATIVE, 0, 500000);	
	gpioWrite(_success_led_pin, 1);
	gpioSleep(PI_TIME_RELATIVE, 0, 500000);
	gpioWrite(_success_led_pin, 0);
}

//Start thread for show_success
void FishTestCamera::_show_success_thread(FishTestCamera* ptr)
{
	ptr->_show_success();
}

//Function invoked to actually start the thread
void FishTestCamera::_call_show_success()
{
	//Tell program it's success
	thread t1(&FishTestCamera::_show_success_thread, this);
	t1.detach();
}
