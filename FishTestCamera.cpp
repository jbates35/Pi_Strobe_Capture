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
	
	//Set up video stream
	_camera.open(0);
	
	//Set video stream width/height
	_camera.set(cv::CAP_PROP_FRAME_WIDTH, _camera_size.width);
	_camera.set(cv::CAP_PROP_FRAME_HEIGHT, _camera_size.height);
	
	//Create unique timestamp for folder
	stringstream timestamp;
	
	//First, create struct which contains time values
	time_t now = time(0);
	tm *ltm = localtime(&now);
	
	//Store stringstream with numbers	
	timestamp << 1900 + ltm->tm_year << "_";
	timestamp << 1 + ltm->tm_mon << "_";
	timestamp << ltm->tm_mday << "_";
	timestamp << ltm->tm_hour << "h";
	timestamp << ltm->tm_min << "m";
	timestamp << ltm->tm_sec << "s";
	
	//Create base file path to build other folders from
	_file_path_base = "./" + timestamp.str() + "/data/";

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
				_camera_state = CAMERA_VIDEO_ON;
				
			}
			
			//If camera state machine is video, can then invoke saving file
			if (_camera_state == CAMERA_VIDEO_ON)
			{
				_camera_state = CAMERA_VIDEO_DONE;
			}
		}
		
		_button_2_pressed = 0;
	}
	
	//State machine for camera
	switch (_camera_state)
	{
	case CAMERA_PICTURE:
		_record_pictures();
		break;
		
	case CAMERA_VIDEO_ON:
		_camera_state = 0;
		break;
			
	case CAMERA_VIDEO_DONE:
		_camera_state = 0;
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


void FishTestCamera::_record_video()
{
}


void FishTestCamera::_save_video()
{
}


void FishTestCamera::_record_pictures()
{
	//Record current directory for creating and then saving pics to
	string curr_file_path = _file_path_picture + to_string(_picture_count) + "/";
	
	//Take picture with flash on and save
	if (_picture_state == 0)
	{
		//Make directory first
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
		
		//Turn LEDs on
		gpioWrite(_flash_leds_pin, 1);
		
		//Sleep 1ms
		gpioSleep(PI_TIME_RELATIVE, 0, 5000);
				
		//Take picture and save
		if (_camera.read(_image)) 
		{			
			cv::imwrite(curr_file_path + to_string(_picture_count) + "_flash_on.jpg", _image);
						
			std::cout << "Image: " << _picture_count << "_flash_on.jpg successfully saved to " << curr_file_path << "\n";
		}
		else
		{
			std::cout << "Error: Image: " << _picture_count << "_flash_on.jpg could not be saved to " << curr_file_path << "\n";
		}
		
		//Advance state machine
		_picture_state++;
		
		//Reset picture timer
		_picture_timer = cv::getTickCount();
		
	}
	
	//Take picture with flash off and save
	if (_picture_state == 1 && (cv::getTickCount() - _picture_timer) / cv::getTickFrequency() >= STROBE_INT)
	{		
		//Turn LEDs off
		gpioWrite(_flash_leds_pin, 0);
		
		//Sleep 1ms
		gpioSleep(PI_TIME_RELATIVE, 0, 5000);
		
		//Take picture and save
		if (_camera.read(_image)) 
		{			
			cv::imwrite(curr_file_path + to_string(_picture_count) + "_flash_off.jpg", _image);
			
			std::cout << "Image: " << _picture_count << "_flash_off.jpg successfully saved to " << curr_file_path << "\n";
		}
		else
		{
			std::cout << "Error: Image: " << _picture_count << "_flash_off.jpg could not be saved to " << curr_file_path << "\n";
		}
		
		//Change permission on all to 777
		system("chmod -R 777 ./");
			
		//Reset state machines so it can exit picture mode
		_picture_state++;
		_camera_state = CAMERA_OFF;
		
		//Increment video count
		_picture_count++;
	}
}