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
#include "mqtt_parser.h"
#include "mqtt.h"
#include "mqtt_wiringpi.h"

/* XXX */

/*typedef static const signed short pin_t;*/
/*static const signed short failure_led_pin = 0;
static const signed short notify_led_pin = 2;*/

gpio_t *failure_gpio;
gpio_t *notify_gpio;
gpio_t *pwr_btn_gpio;
gpio_t *tip120_gpio;



int
gpios_setup(gpios_t *gp_set)
{
	gpio_t *gp;
	int 	cnt=0;
	if (gp_set == NULL) 
		return (-1);

	if ((gp = gp_set->gpios_head) == NULL) {
		return (-1);
	}
	do {
		if (gp != NULL) {
			switch(gp->g_type) {
				case G_PWR_BTN:
					pinMode(gp->g_pin, INPUT);
					pwr_btn_gpio = gp;
				break;
				case G_LED_FAILURE:
					failure_gpio = gp;
					pinMode(gp->g_pin, OUTPUT);
				break;
				case G_LED_NOTIFY:
					notify_gpio = gp;
					pinMode(gp->g_pin, OUTPUT);
				break;
					
			}
		}
		gp = gp->g_next;
	} while (gp != gp_set->gpios_head && gp != NULL);
	/*failure_led_pin */
	
	return (cnt);
}

int 
startup_led_act(int ledticks, int blink_delay) 
{
	int n = 0;
	short ticktack = 1;
	short failure_pin, notify_pin;

	if (notify_gpio == NULL || failure_gpio == NULL) {
		return (0);
	}
	notify_pin = notify_gpio->g_pin;
	failure_pin = failure_gpio->g_pin;

	digitalWrite(notify_pin, HIGH);
	digitalWrite(failure_pin, HIGH);


	for (n = 0; n < ledticks; n++ ) {
		if (ticktack) { 
			digitalWrite(notify_pin, LOW);
			delay(blink_delay*3);
			digitalWrite(notify_pin, HIGH);
			delay(blink_delay);
			ticktack = 0;
		} else {
			digitalWrite(failure_pin, LOW);
			delay(blink_delay*3);
			digitalWrite(failure_pin, HIGH);
			delay(blink_delay);
			ticktack = 1;
		}

	}
	digitalWrite(failure_pin, LOW);
	digitalWrite(notify_pin, LOW);
}

int startup_fanctl()
{
	int n = 0;
	
	if (tip120_gpio == NULL)
		return (-1);
	pinMode(tip120_gpio->g_pin, PWM_OUTPUT);
	pwmWrite(tip120_gpio->g_pin, 100);
}


int 
term_led_act(short failure) 
{

	if (notify_gpio != NULL)
		digitalWrite(notify_gpio->g_pin, LOW);
	if (failure && failure_gpio != NULL)
		digitalWrite(failure_gpio->g_pin, HIGH);
}

int
flash_led(short ledid, int act) {
	int ret = 0;

	switch(ledid) {
		case FAILURE_LED:
			if (failure_gpio == NULL) {
				return (-1);
			}
			digitalWrite(failure_gpio->g_pin, act);
			break;
		case NOTIFY_LED:
			if (notify_gpio == NULL) {
				return (-1);
			}
			digitalWrite(notify_gpio->g_pin, act);
			break;
		default:
			ret = -1;
	}
	return (ret);
}

int
fanctl(short act, int *args) 
{
	if (tip120_gpio == NULL) {
		return (-1);
	}
	pinMode(tip120_gpio->g_pin, PWM_OUTPUT);
	int ret = 0;
	switch(act) {
		case FAN_ON:
			fprintf(stderr, "putting HIGH %d\n", tip120_gpio->g_pin);
			pwmWrite(tip120_gpio->g_pin, 1024);
			break;
		case FAN_OFF:
			pwmWrite(tip120_gpio->g_pin, 0);
			break;
		default:
			ret = -1;
	}
	return (ret);
}
