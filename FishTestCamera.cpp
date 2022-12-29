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
}

FishTestCamera::~FishTestCamera()
{
}


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
	_file_path_base = "./" + current_time + "/data/";

	//Path for video and picture files and create directories
	_file_path_video = _file_path_base + "video/";
	_file_path_picture = _file_path_base + "pictures/";
	
	//Make command for VIDEO directory and then convert it to char
	string file_command = "mkdir -p " + _file_path_video;
	char *file_command_char;
	file_command_char = (char*) malloc(file_command.size() + 1);
	
	for (int char_ind = 0; char_ind < file_command.size(); char_ind++)
	{
		file_command_char[char_ind] = file_command[char_ind];
	}
	file_command_char[file_command.size()] = '\0';
	
	//Make directory, return error if not possible
	if (system(file_command_char) == -1)
	{
		std::cout << "Failed to make video directory, exiting program\n";
		return -1;
	}
	
	//Now make command for PICTURE directory and convert it to char
	file_command = "mkdir -p " + _file_path_picture;
	file_command_char = (char*) malloc(file_command.size() + 1);
	
	for (int char_ind = 0; char_ind < file_command.size(); char_ind++)
	{
		file_command_char[char_ind] = file_command[char_ind];
	}
	file_command_char[file_command.size()] = '\0';
	
	//Make directory, return error if not possible
	if (system(file_command_char) == -1)
	{
		std::cout << "Failed to make picture directory, exiting program\n";
		return -1;
	}
	
	free(file_command_char);
	
	//Now change the two directories to chmod +777	
	if (system("chmod -R 777 ./") == -1)
	{
		std::cout << "Failed to change permissions, exiting program\n";
		return -1;			
	}
	
	//Initialize pic and video count
	_picture_count = 0;
	_video_count = 0;
		
	//Intialize camera state
	_camera_state = CAMERA_OFF;
	_picture_state = 0;
	
	//Initialize LED state
	_led_state = false;
	
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


void FishTestCamera::run()
{
	//If button 2 has been pressed, turn second debounce on
	if (_button_2_pressed == 1 && (cv::getTickCount() - _button_2_timer) / cv::getTickFrequency() >= VID_BUTTON_INTERVAL)
	{
		//Make sure button is pressed still (after 2 seconds)
		if (gpioRead(_button_2_pin) == false)
		{	
			//If camera state machine is off, can turn video mode on
			if (_camera_state == CAMERA_OFF)
			{
				//Set camera state
				_camera_state = CAMERA_VIDEO;
				
				//Set video state to 'record'
				_video_state = VIDEO_RECORD;
			}
			
			//If camera state machine is video, can then invoke saving file
			if (_camera_state == CAMERA_VIDEO)
			{
				//Change video state to tell script to end file, save it, etc.
				_video_state = VIDEO_DONE;
			}
		}
		
		_button_2_pressed = 0;
	}
	
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
	//Start timer
	_button_2_timer = cv::getTickCount();
	
	//Turn button on 
	_button_2_pressed = 1;
}


//Turn flash on, take picture, turn flash off, take picture, save files
void FishTestCamera::_camera_off()
{
	//Make sure camera is running
	if (_camera.isOpened() == false)
	{
		return;
	}
	
	//Load camera to frame
	_camera.read(_image);
	
	//Show camera for preview
	cv::imshow("Preview camera", _image);
	cv::waitKey(10);
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
		double fps = 20.0;
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
	}
		
	//Record video and show frames
	while (_video_state == VIDEO_RECORD)
	{
		//Initialize timer for grabbing frame time
		_frame_timer = cv::getTickCount();
				
		//Toggle led state and strobe lights
		_led_state = !_led_state;
		gpioWrite(_flash_leds_pin, _led_state);
		
		//Return if blank frame grabbed
		if (!_camera.read(_image))
		{
			_file_info_ss << "WARNING: Grabbed blank frame, ending video...\n";
			
			break;
		}
		
		//Increment frame count, get time, write to log
		_frame_count++;
		_file_info_ss << "Frame " << _frame_count << " time: " << round(1000*(cv::getTickCount() - _frame_timer) / cv::getTickFrequency()) << "ms \n";
		
		
	}
		
	//Save video file to file
	
	//Write remaining file info
	_file_info_ss << "File " << _video_count << ".avi successfully saved to " << _file_path_video << "\n";
	_file_info_ss << "Length of video: " << (cv::getTickCount() - _video_timer) / cv::getTickFrequency() << "s\n";
	_file_info_ss << "Date and time of video record: " << _get_time() << "\n\n";
		
	//Video is done recording, so can turn blue LEDs off
	gpioWrite(_video_led_pin, 0);	
		
	//Reset camera state
	_camera_state = CAMERA_OFF;
		
	//Increment video count
	_video_count++;
		
	//Write to log file
	_write_file(_file_info_ss.str(), _file_path_video + to_string(_video_count) + "_log.txt");
	
	//Reset video state
	_video_state = VIDEO_RECORD;

	//Show camera for preview
	cv::imshow("Preview camera", _image);
	cv::waitKey(10);	
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
		_camera.open(0);
		
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
