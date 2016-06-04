
/*
 * $Id$
 */
#ifndef _MQTT_SENSORS_H_
#define  _MQTT_SENSORS_H_

#define PIN_BASE 220
extern int sensors_init(void);
extern int pcf8591p_ain(unsigned int);
extern float get_temperature(const char *);
extern double get_pressure(void);
extern void bmp85_init(void);

#endif /*  ! _MQTT_SENSORS_H_ */
