
/*
 * $Id$
 */
#ifndef _MQTT_SENSORS_H_
#define  _MQTT_SENSORS_H_

struct sensor_set {
	int 	pcf_8591_pin_base;
	struct mq_sensor 	*s_first;
};
#define PIN_BASE 220
extern int sensors_init(void);
extern int pcf8591p_ain(unsigned int);
extern float get_temperature(const char *);
extern double get_pressure(void);
extern void bmp85_init(void);


#endif /*  ! _MQTT_SENSORS_H_ */
