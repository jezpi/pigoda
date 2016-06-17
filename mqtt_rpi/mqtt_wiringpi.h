#ifndef _MQTT_WIRINGPI_H_
#define  _MQTT_WIRINGPI_H_

#define RED_LED 0x1
#define GREEN_LED 0x2
#define FAN_ON 0x1
#define FAN_OFF 0x2
extern int startup_fanctl();
extern int fanctl(short, int *);
extern int startup_led_act(int);
extern int term_led_act(short) ;
extern int flash_led(short, int);

#endif /*  !_MQTT_WIRINGPI_H_ */
