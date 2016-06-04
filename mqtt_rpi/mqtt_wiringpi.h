#ifndef _MQTT_WIRINGPI_H_
#define  _MQTT_WIRINGPI_H_

#define FAN_ON 0x1
#define FAN_OFF 0x2
extern int startup_fanctl();
extern int fanctl(short, int *);
extern int startup_led_act();
extern int term_led_act() ;

#endif /*  !_MQTT_WIRINGPI_H_ */
