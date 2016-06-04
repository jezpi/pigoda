/*
 * $Id$
 */
#ifndef _MQTTC_H_
#define  _MQTTC_H_

typedef struct mqtt_global_config_t {
	const char *pidfile;
	const char *logfile;
	unsigned int debug_level;
	const char *mqtt_host;
	const char *mqtt_user;
	const char *mqtt_password;
	unsigned short mqtt_port;
	char *mqtt_channels[100];
	
} mqtt_global_cfg_t;

#define DEFAULT_CONFIG_FILE	"mqtt_rpi.yaml"
#define HOST_NAME_MAX	64
extern int parse_configfile(const char *, mqtt_global_cfg_t *);

#endif /*  ! _MQTTC_H_ */
