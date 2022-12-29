/**
 * @function findContours_Demo.cpp
 * @brief Demo code to find contours in an image
 * @author OpenCV team
 */

#include <pigpio.h>

#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#include <opencv2/opencv.hpp>

#include "FishTestCamera.h"

#define BUTTON_1_PIN	19 // pin for button
#define BUTTON_2_PIN	13 // pin for button

#define FLASH_LEDS_PIN	26 // pin for flashing led
#define VIDEO_LED_PIN	20 // pin for flashing led
#define SUCCESS_LED_PIN	21 // pin for flashing led
	
#define DEBOUNCE_INTERVAL			10000	// microseconds
#define DEBOUNCE_INTERVAL_VID_S		1		//seconds
#define DEBOUNCE_INTERVAL_VID_US	250000	// microseconds


//Holds camera object for taking picture, saving to file, doing some processing
FishTestCamera cam(FLASH_LEDS_PIN, VIDEO_LED_PIN, SUCCESS_LED_PIN, BUTTON_1_PIN, BUTTON_2_PIN);
	
//////////FUNCTION PROTOTYPES///////////
//Initializes button ISRs
void init();

//Debounces and invokes camera object ISR 1 if pressed
void button_1_isr(int gpio, int level, uint32_t tick);

//Debounces and invokes camera object ISR 2 if pressed
void button_2_isr(int gpio, int level, uint32_t tick);

//////////FUNCTION DEFINITIONS///////////
int main()
{

	//Initialize variables, pins, etc.
	if (cam.init() < 0)
	{
		std::cout << "Initialize unsuccessful";
		return -1;
	}
	
	//Initializes buttons
	init();
	
	//Continuous program loop
	while (1)
	{
		cam.run();
	}
}

void init()
{
	//Button 1 setup
	gpioSetMode(BUTTON_1_PIN, PI_INPUT);
	gpioSetPullUpDown(BUTTON_1_PIN, PI_PUD_UP);
		
	//Setup ISR with Button 1 
	gpioSetISRFunc(BUTTON_1_PIN, FALLING_EDGE, ISR_TIMEOUT, button_1_isr);	

	//Button 2 setup
	gpioSetMode(BUTTON_2_PIN, PI_INPUT);
	gpioSetPullUpDown(BUTTON_2_PIN, PI_PUD_UP);
	
	//Setup ISR with Button 2 
	gpioSetISRFunc(BUTTON_2_PIN, FALLING_EDGE, ISR_TIMEOUT, button_2_isr);
}

//Button 1 ISR - Debounces, invokes camera ISR 1 when pressed
void button_1_isr(int gpio, int level, uint32_t tick)
{
	gpioSleep(PI_TIME_RELATIVE, 0, DEBOUNCE_INTERVAL);	
	
	if (gpioRead(BUTTON_1_PIN) == false)
	{
		cam.set_button_1();
	}
}

//Button 2 ISR - Debounces, invokes camera ISR 2 when pressed
void button_2_isr(int gpio, int level, uint32_t tick)
{
	gpioSleep(PI_TIME_RELATIVE, DEBOUNCE_INTERVAL_VID_S, DEBOUNCE_INTERVAL_VID_US);	
	
	if (gpioRead(BUTTON_2_PIN) == false)
	{
		cam.set_button_2();		
	}
}