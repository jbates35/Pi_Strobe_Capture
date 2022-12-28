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

//Holds camera object for taking picture, saving to file, doing some processing
#include "FishTestCamera.h"

#define BUTTON_1_PIN	19 // pin for button
#define LED_PIN			26 // pin for led

#define ISR_TIMEOUT		100000 // microseconds

#define DEBOUNCE_INTERVAL 0.01 // Amount of time to debounce (10ms)


/*******	FUNCTION PROTOCOLS	*******/

//Returns 0 if successful
int init();

//ISR for Button 1
void button_1_isr(int gpio, int level, uint32_t tick);

//
void camera_shot();



/*******	FUNCTION DEFINTIONS		*******/

int main()
{
	if (init() < 0)
	{
		std::cout << "Initialize unsuccessful";
		return -1;
	}
	
	while (1)
	{
		unsigned int led_val = gpioRead(BUTTON_1_PIN);
	}	
}

//Initailizes pigpio and its pins
int init()
{
	//Make sure program can run in first place
	if (gpioInitialise() < 0)
	{
		return -1;
	}
	
	//LED Pin setup
	gpioSetMode(LED_PIN, PI_OUTPUT);
	gpioWrite(LED_PIN, 0);

	//Button 1 setup
	gpioSetMode(BUTTON_1_PIN, PI_INPUT);
	gpioSetPullUpDown(BUTTON_1_PIN, PI_PUD_UP);
	
	//Setup ISR with Button 1 
	gpioSetISRFunc(BUTTON_1_PIN, FALLING_EDGE, ISR_TIMEOUT, button_1_isr);	
	
	return 0;
}

//Set ISR for button 1
void button_1_isr(int gpio, int level, uint32_t tick)
{	
	//Test counter
	static bool button_pressed = false;
	static double press_time = cv::getTickCount();
	static bool led_state=false;

	//If button is in debounce interval, return and do nothing
	if (button_pressed == true && (cv::getTickCount() - press_time) / cv::getTickFrequency() < DEBOUNCE_INTERVAL)
	{
		return;
	}
	
	//If button is pressed, make true and start timer
	if (button_pressed == false) 
	{
		button_pressed = true;
		
		//Start timer
		press_time = cv::getTickCount();
		
		return;
	}
	
	//If debounce successful, run actual function
	if (button_pressed == true && (cv::getTickCount() - press_time) / cv::getTickFrequency() >= DEBOUNCE_INTERVAL)
	{
		button_pressed = false;
		
		led_state = !led_state;
		
		gpioWrite(LED_PIN, !gpioRead(LED_PIN));
	}	
}