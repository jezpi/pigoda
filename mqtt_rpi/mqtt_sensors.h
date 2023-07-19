
/*
 * $Id$
 */
#ifndef _MQTT_SENSORS_H_
#define  _MQTT_SENSORS_H_

#define PIN_BASE 220
extern int sensors_init(sensors_t *);
extern float pcf8591p_ain(char *);
extern bool get_temperature(const char *, double *);
extern bool get_pressure(double *);
extern bool get_sht30(double *);
extern void bmp85_init(void);


#endif /*  ! _MQTT_SENSORS_H_ */
