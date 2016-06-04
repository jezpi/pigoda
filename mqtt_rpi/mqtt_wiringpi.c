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


int 
startup_led_act() 
{
	int n = 0;
	pinMode(2, OUTPUT);
	digitalWrite(2, HIGH);
	for (n = 0; n < 5; n++ ) {
		digitalWrite(2, LOW);
		delay(200);
		digitalWrite(2, HIGH);
		delay(200);
	}
}
#define FAN_PIN 1
int startup_fanctl()
{
	int n = 0;
	pinMode(FAN_PIN, PWM_OUTPUT);
	pwmWrite(FAN_PIN, 100);
}


int 
term_led_act() 
{
	digitalWrite(2, LOW);
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
