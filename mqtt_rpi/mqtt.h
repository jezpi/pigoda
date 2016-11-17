/*
 * $Id$
 */
#ifndef _MQTT_H_
#define  _MQTT_H_
typedef enum {SENS_W1, SENS_I2C, SENS_DHTXX} stype_t;   
typedef enum {I2C_PCF8591P, I2C_BMP85} i2ctype_t;   

typedef enum {LED_FAILURE, LED_NOTIFY} ledact_t;

typedef struct led {
	char 	*l_name;
	int 	l_pin; 	
	ledact_t 	l_action;
	struct led *l_next;
} led_t;

led_t 	*curled;

typedef struct ledset {
	led_t 	*ls_head;
	led_t 	*ls_tail;
} ledset_t;


struct sensor {
	char *s_name;
	char *s_channel;
	stype_t 	s_type;
	i2ctype_t 	s_i2ctype;
	char *s_config;
	char *s_address;
	struct sensor *s_next;
} ;
typedef struct sensor sensor_t ;

typedef struct sensors {
	int 		sn_count;
	sensor_t 	*sn_head;
	sensor_t 	*sn_tail;
	
} sensors_t;
typedef struct mqtt_global_config_t {
	const char *pidfile;
	const char *logfile;
	char 		*identity;
	short		daemon;
	unsigned int debug_level;
	const char *mqtt_host;
	const char *mqtt_user;
	const char *mqtt_password;
	unsigned short mqtt_port;
	unsigned int pool_sensors_delay;
	sensors_t 	*sensors;
	ledset_t 	*leds;
} mqtt_global_cfg_t;


#define DEFAULT_CONFIG_FILE	"mqtt_rpi.yaml"
#define HOST_NAME_MAX	64
extern int parse_configfile(const char *, mqtt_global_cfg_t *);

/* Missing constants */
#define CONNACK_ACCEPTED 0
#define CONNACK_REFUSED_PROTOCOL_VERSION 1
#define CONNACK_REFUSED_IDENTIFIER_REJECTED 2
#define CONNACK_REFUSED_SERVER_UNAVAILABLE 3
#define CONNACK_REFUSED_BAD_USERNAME_PASSWORD 4
#define CONNACK_REFUSED_NOT_AUTHORIZED 5

#endif /*  ! _MQTT_H_ */
