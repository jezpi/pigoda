#ifndef _MQTT_PARSER_H_
#define  _MQTT_PARSER_H_

typedef enum {SENS_W1, SENS_I2C, SENS_DHTXX} stype_t;   
typedef enum {I2C_PCF8591P, I2C_BMP85, I2C_SHT30} i2ctype_t;   
typedef enum {G_LED_FAILURE, G_LED_NOTIFY, G_PWR_BTN, G_PIR_SENSOR, G_TIP120, G_RELAY, G_RESERVED} gpio_type_t;

typedef struct gpio {
	char 		*g_name;
	char		*g_topic;
	int 		 g_pin; 
	gpio_type_t  	 g_type;
	struct gpio 	*g_next;
} gpio_t;


typedef struct gpioset {
	gpio_t 	*gpios_head;
	gpio_t 	*gpios_tail;
} gpios_t;

typedef enum {SENS_FAIL, SENS_OK, SENS_INIT} sens_state_t;
struct sensor {
	char *s_name;
	char *s_channel;
	stype_t 	s_type;
	i2ctype_t 	s_i2ctype;
	char *s_config;
	char *s_address;
	sens_state_t	s_st;
	struct sensor *s_next;
} ;
typedef struct sensor sensor_t ;

typedef struct sensors {
	int 		sn_count;
	short		sn_adc_configured;
	sensor_t 	*sn_head;
	sensor_t 	*sn_tail;
	
} sensors_t;

struct pir_config;
typedef struct mqtt_global_config_t {
	const char 	*pidfile;
	const char 	*logfile;
	char 		*identity;
	short		 daemon;
	unsigned int 	 debug_level;
	const char 	*mqtt_host;
	const char 	*mqtt_user;
	const char 	*mqtt_password;
	unsigned short 	 mqtt_port;
	unsigned int	 mqtt_keepalive;
	unsigned int 	 pool_sensors_delay;
	sensors_t 	*sensors;
	gpios_t 	*gpios;
	struct pir_config *mqtt_pir;
} mqtt_global_cfg_t;


extern int parse_configfile(const char *, mqtt_global_cfg_t *);

extern gpio_t * new_gpio(gpios_t *, char *);
extern gpio_t *gpiopin_by_type(gpios_t *, gpio_type_t , char *);

#endif /*  ! _MQTT_PARSER_H_ */
