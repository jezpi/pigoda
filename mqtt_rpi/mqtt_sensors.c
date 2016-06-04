/*
 * =====================================================================================
 *
 *       Filename:  mqtt_sensors.c
 *
 *    Description:  sensor reading code
 *
 *        Version:  1.0
 *        Created:  07/05/16 20:19:25
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
#include <stdlib.h>
#include <unistd.h>

#include <wiringPi.h>
#include <mcp23017.h>
#include <pcf8591.h>
#include <errno.h>

#include "mqtt_sensors.h"
#include "bmp85/bmp85.h"

int sensors_init(void) {

	wiringPiSetup();
	
	pcf8591Setup(PIN_BASE, 0x48);

}

int pcf8591p_ain(unsigned int pin) {
	int ret;
	ret = analogRead(PIN_BASE+pin);
	return (ret);
}


#define W1_DEVS_PATH "/sys/bus/w1/devices/"

static char *getraw(const char *devpath)
{
	FILE	*f;
	char	rbuf[BUFSIZ], *chp, *saveptr;
	char	*tempstr = NULL;

	if ((f = fopen(devpath, "r")) == NULL) {
		fclose(f);
		return (NULL);
	}

	while ((chp = fgets(rbuf, sizeof rbuf, f)) != NULL) {
		for ( chp = strtok(rbuf, " \t\n", &saveptr) ; chp != NULL; chp = strtok(saveptr, " \t\n", &saveptr))
		{
			if (!strncmp(chp, "t=", 2)) {
				tempstr = strdup((chp+2));
			}
		}
	}
	fclose(f);
	return (tempstr);
}

float
get_temperature(const char *ds_name)
{
	char	pathbuf[BUFSIZ];
	char *eptr, *chp;
	float	temp;

	snprintf(pathbuf, sizeof pathbuf, "/sys/bus/w1/devices/%s/w1_slave", ds_name);
	if ((chp = getraw(pathbuf)) == NULL) {
		fprintf(stderr, "failed to read %s %s\n", pathbuf, strerror(errno));
		return (-1);
	}
	

	temp = (float) ((float)strtol(chp,  &eptr, 10)/1000.00);
	return (temp);

}

double
get_pressure(void)
{
	struct bmp85 *dta;
	dta = bmp85_getdata();
	return (dta->pressure);
}

