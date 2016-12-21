#ifndef _MQTT_WIRINGPI_H_
#define  _MQTT_WIRINGPI_H_

#define FAILURE_LED 0x1
#define NOTIFY_LED 0x2
#define FAN_ON 0x1
#define FAN_OFF 0x2


extern int gpios_setup(gpios_t *);

extern int startup_fanctl();
extern int fanctl(short, int *);
extern int startup_led_act(int, int);
extern int term_led_act(bool) ;
extern int flash_led(short, int);
extern int poll_pwr_btn();
extern int relay_ctl(int);

#endif /*  !_MQTT_WIRINGPI_H_ */
