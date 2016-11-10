/*
 * $Id$
 */
#ifndef _MQTT_H_
#define  _MQTT_H_

typedef struct mqtt_global_config_t {
	const char *pidfile;
	const char *logfile;
	short		daemon;
	unsigned int debug_level;
	const char *mqtt_host;
	const char *mqtt_user;
	const char *mqtt_password;
	unsigned short mqtt_port;
	char *mqtt_channels[100];
	unsigned int pool_sensors_delay;
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
