/*
 * =====================================================================================
 *
 *       Filename:  mqtt_wiringpi.c
 *
 *    Description:  wiringPi 
 *
 *        Version:  1.0
 *        Created:  06/01/2015 04:10:25 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Jez (JJ), jez@obin.org
 *   Organization:  
 *
 * =====================================================================================
 */


#include <sys/types.h>
#include <stdio.h>

#include <wiringPi.h>
#include "mqtt_wiringpi.h"
/*
int
main(int argc, char **argv)
{
	char *cmd = NULL;
	int pin = 6;

  wiringPiSetup ();
  pinMode(pin, OUTPUT);

  if (argc > 1) {
	  cmd = argv[1];
  }
  if (cmd == NULL) {
	  printf("unknown command!\n");
	  cmd = "NULL";
  } else if (!strncmp(cmd, "high", 4)) {

  	digitalWrite(pin, HIGH);
  } else if (!strncmp(cmd, "low", 3)) {
  	digitalWrite(pin, LOW);
  } 

  printf("pin %d set %s \tRev. %d\n", pin, cmd, piBoardRev());
	return (0);
}
*/
static const unsigned short red_led_pin = 0;
static const unsigned short green_led_pin = 2;

int 
startup_led_act(int ledticks) 
{
	int n = 0;
	short ticktack = 1;
	pinMode(green_led_pin, OUTPUT);
	pinMode(red_led_pin, OUTPUT);
	digitalWrite(green_led_pin, HIGH);
	digitalWrite(red_led_pin, HIGH);
	for (n = 0; n < ledticks; n++ ) {
		if (ticktack) { 
			digitalWrite(green_led_pin, LOW);
			delay(200);
			digitalWrite(green_led_pin, HIGH);
			delay(200);
			ticktack = 0;
		} else {
			digitalWrite(0, LOW);
			delay(200);
			digitalWrite(0, HIGH);
			delay(200);
			ticktack = 1;
		}

	}
	digitalWrite(red_led_pin, LOW);
	digitalWrite(green_led_pin, LOW);
}
#define FAN_PIN 1
int startup_fanctl()
{
	int n = 0;
	pinMode(FAN_PIN, PWM_OUTPUT);
	pwmWrite(FAN_PIN, 100);
}


int 
term_led_act(short failure) 
{
	digitalWrite(green_led_pin, LOW);
	if (failure)
		digitalWrite(red_led_pin, HIGH);
}

int
flash_led(short ledid, int act) {
	int ret = 0;
	switch(ledid) {
		case RED_LED:
			digitalWrite(red_led_pin, act);
			break;
		case GREEN_LED:
			digitalWrite(green_led_pin, act);
			break;
		default:
			ret = -1;
	}
	return (ret);
}

int
fanctl(short act, int *args) 
{
	pinMode(FAN_PIN, PWM_OUTPUT);
	int ret = 0;
	switch(act) {
		case FAN_ON:
			fprintf(stderr, "putting HIGH %d\n", FAN_PIN);
			pwmWrite(FAN_PIN, 1024);
			break;
		case FAN_OFF:
			pwmWrite(FAN_PIN, 0);
			break;
		default:
			ret = -1;
	}
	return (ret);
}
