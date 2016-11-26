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
#include <string.h>

#include "mqtt_parser.h"
#include "mqtt.h"
#include "mqtt_sensors.h"
#include "bmp85/bmp85.h"

#define W1_DEVS_PATH "/sys/bus/w1/devices/"

int 
sensors_init(sensors_t *sn) 
{
	sensor_t 	*sp;
	char 		*endptr;
	long 		i2caddress;
	int 		scnf = 0;

	wiringPiSetup();
	sp = sn->sn_head;
	do {
		if (sp->s_st != SENS_INIT || (sp->s_type == SENS_I2C && sn->sn_adc_configured > 0)) {
			if (sn->sn_adc_configured > 0) {
				sp->s_st = SENS_OK;
			}
			sp = sp->s_next;
			continue;
		}
		switch(sp->s_type) {
			case SENS_I2C:
				switch(sp->s_i2ctype) {
					case I2C_PCF8591P:
						i2caddress = strtol(sp->s_address, &endptr, 16);
						pcf8591Setup(PIN_BASE, i2caddress);
						sp->s_st = SENS_OK;
						sn->sn_adc_configured = 1;
						break;
					case I2C_BMP85:
						bmp85_init();
						sp->s_st = SENS_OK;
						/* MQTT_log */
						break;
					default:
						return (-1);
				}
				scnf++;
			break;
			case SENS_W1:
				sp->s_st = SENS_OK;
				/* NOP */
				scnf++;
			break;
			default:
				return (-1);
			
		}
		sp = sp->s_next;
	} while (sp != NULL && sp != sn->sn_head);
	

	return (scnf);
}

float pcf8591p_ain(char *pinnum) {
	float ret;
	int  pin;
	char *endptr;

	pin = (int) strtol(pinnum, &endptr, 10);
	ret = (float) analogRead(PIN_BASE+pin);
	return (ret);
}



static char *
getraw(const char *devpath)
{
	FILE	*f;
	char	rbuf[BUFSIZ], *chp, *saveptr;
	char	*tempstr = NULL;

	if ((f = fopen(devpath, "r")) == NULL) {
		return (NULL);
	}

	while ((chp = fgets(rbuf, sizeof rbuf, f)) != NULL) {
		for ( chp = strtok_r(rbuf, " \t\n", &saveptr) ; chp != NULL; chp = strtok_r(saveptr, " \t\n", &saveptr))
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

