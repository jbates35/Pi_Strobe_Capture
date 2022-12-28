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

#define BUTTON_1_PIN	19
#define LED_PIN			26

//Returns 0 if successful
int init();

//ISR for Button 1
void button_1_isr();

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
		gpioWrite(LED_PIN, led_val);
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

	//Button setup
	gpioSetMode(BUTTON_1_PIN, PI_INPUT);
	gpioSetPullUpDown(BUTTON_1_PIN, PI_PUD_UP);
	
	return 0;
}

//Takes picture of camera
void button_1_isr()
{
	
}