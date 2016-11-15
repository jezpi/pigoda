#include <stdio.h>

#include <yaml.h>
#include "mqtt.h"

#ifdef DEBUG

unsigned short DEBUG_FLAG;
/*  /static unsigned short DEBUG_MODE=0x0;*/
#define dprintf if (DEBUG_FLAG) printf
#define ddprintf if (DEBUG_FLAG>0x4) printf
#endif
typedef enum {DEBUG_BASE=0x1, DEBUG_FUNC=0x2} debug_type_t;
/*
typedef struct mqtt_global_config_t {
	const char *pidfile;
	int debug_level;
	const char *mqtt_host;
	const char *mqtt_user;
	const char *mqtt_password;
} mqtt_global_cfg_t;

*/



sensor_t *
sensor_new(sensors_t *snrs, char *name)
{
	sensor_t 	*snew;

	snew = calloc(1, sizeof(sensor_t));
	snew->s_name = strdup(name);
	if (snrs->sn_head == NULL) {
		snrs->sn_head = snrs->sn_tail = snew;
	} else {
		snrs->sn_tail->s_next = snew;
		snrs->sn_tail = snew;
	}
	return (snew);
}
sensor_t *
sensor_find_by_name(sensors_t *snrs, const char *name)
{
	sensor_t 	*ret = NULL, *sp;
	
	if (snrs->sn_head == NULL)
		return (NULL);

	do {
		if (!strcasecmp(name, sp->s_name)) {
			ret = sp;
		}
		sp = sp->s_next;
	} while ( snrs->sn_head != sp && sp != NULL);
	return (ret);
}


sensors_t sensors;

enum {SCALAR_SSEQ, SCALAR_SMAP, SCALAR_MAIN} scalar_opts = SCALAR_MAIN;

mqtt_global_cfg_t myMQTT;

static char *curvar;
FILE *debuglog;
enum {V_UNKNOWN, V_PIDFILE, V_LOGFILE, V_DEBUG, V_SERVER, V_MQTTUSER, V_MQTTPASS, V_MQTTPORT, V_DAEMON, V_DELAY} c_opts = V_UNKNOWN;


enum {S_MODNAME, S_CHANNEL, S_MODULE, S_TYPE, S_CONFIG, S_VAR , S_ADDRESS, S_I2CTYPE, S_INIT} module_opts = S_INIT;
enum {BLK_MAIN, BLK_SENSORS, BLK_INPUT} blk_opts = BLK_MAIN;

int
proc_main_opt(char *scalar_value)
{
	if (DEBUG_FLAG&0x4) fprintf(debuglog, "%s() : Processing  %s\n", __func__ , scalar_value);
	if (!strcasecmp(scalar_value, "pidfile")) {
		c_opts = V_PIDFILE;
		myMQTT.pidfile = NULL;
	} else if (!strcasecmp(scalar_value, "debug")) {
		c_opts = V_DEBUG;
	} else if (!strcasecmp(scalar_value, "delay")) {
		c_opts = V_DELAY;
	} else if (!strcasecmp(scalar_value, "mqtt_user")) {
		c_opts = V_MQTTUSER;
	} else if (!strcasecmp(scalar_value, "logfile")) {
		c_opts = V_LOGFILE;
	} else if (!strcasecmp(scalar_value, "mqtt_password")) {
		c_opts = V_MQTTPASS;
	} else if (!strcasecmp(scalar_value, "mqtt_port")) {
		c_opts = V_MQTTPORT;
	} else if (!strcasecmp(scalar_value, "mqtt_host")) {
		c_opts = V_SERVER;
	} else if (!strcasecmp(scalar_value, "daemon")) {
		c_opts = V_DAEMON;

	} else {
		
		switch (c_opts) {
			case V_DAEMON:
				if (!strncmp(scalar_value, "true", 4)) {
					myMQTT.daemon = 1;
				} else
					myMQTT.daemon = 0;
				c_opts = V_UNKNOWN;
				break;
			case V_PIDFILE:
				myMQTT.pidfile = strdup(scalar_value);
				c_opts = V_UNKNOWN;
				break;
			case V_DEBUG:
				myMQTT.debug_level = (unsigned short) atoi(scalar_value);
				c_opts = V_UNKNOWN;
				break;
			case V_DELAY:
				myMQTT.pool_sensors_delay = (unsigned int) atoi(scalar_value);
				c_opts = V_UNKNOWN;
				break;
			case V_SERVER:
				myMQTT.mqtt_host = strdup(scalar_value);
				c_opts = V_UNKNOWN;
				break;
			case V_MQTTUSER:
				myMQTT.mqtt_user = strdup(scalar_value);
				c_opts = V_UNKNOWN;
				break;
			case V_MQTTPORT:
				myMQTT.mqtt_port = atoi(scalar_value);
				c_opts = V_UNKNOWN;
				break;
			case V_MQTTPASS:
				myMQTT.mqtt_password = strdup(scalar_value);
				c_opts = V_UNKNOWN;
				break;
			case V_LOGFILE:
				myMQTT.logfile = strdup(scalar_value);
				c_opts = V_UNKNOWN;
				break;

		}
	}

}
/*
enum {S_MODNAME, S_CHANNEL, S_MODULE, S_VAR , S_INIT} module_opts = S_INIT;
enum {BLK_MAIN, BLK_SENSORS, BLK_INPUT} blk_opts = BLK_MAIN;
*/
sensor_t *cursensor;
int
proc_sensors_opt(char *scalar_value)
{
	switch (module_opts) {
		case S_INIT:

			if (!strcasecmp(scalar_value, "name")) {
				module_opts = S_MODNAME;
			} else if (!strcasecmp(scalar_value, "channel")) {
				module_opts = S_CHANNEL;

			} else if (!strcasecmp(scalar_value, "module")) {
				module_opts = S_MODNAME;
			}else if (!strcasecmp(scalar_value, "type")) {
				module_opts = S_MODULE;
			}else if (!strcasecmp(scalar_value, "address")) {
				module_opts = S_ADDRESS;
			}else if (!strcasecmp(scalar_value, "i2ctype")) {
				module_opts = S_I2CTYPE;
			} else if (!strcasecmp(scalar_value, "config")) {
				module_opts = S_CONFIG;
			} else {
				fprintf(stderr, "Unknown scalar %s\n", scalar_value);
			}

			break;
		case S_MODNAME:
				cursensor = sensor_new(&sensors, scalar_value);
				fprintf(stderr, "new sensor %s\n", scalar_value);
				module_opts = S_INIT;
			break;
		case S_CHANNEL:
			if (cursensor != NULL) {
				cursensor->s_channel = strdup(scalar_value);
			} else {
				return -1;
			}
				module_opts = S_INIT;
			break;
		case S_TYPE:
			if (cursensor != NULL) {
				cursensor->s_channel = strdup(scalar_value);
			} else {
				return -1;
			}
				module_opts = S_INIT;
			break;
		case S_CONFIG:
			if (cursensor != NULL) {
				cursensor->s_config = strdup(scalar_value);
			} else {
				return -1;
			}

			module_opts = S_INIT;
			break;
		case S_MODULE:
			if (!strcasecmp("DS18B20", scalar_value)) {
				cursensor->s_type = SENS_W1;
			} else if (!strcasecmp("i2c", scalar_value)) {
				cursensor->s_type = SENS_I2C;
			} else if (!strcasecmp("pcf8591p", scalar_value)) {
				cursensor->s_type = SENS_I2C;
			} else {
				printf("Unsupported module type %s\n", scalar_value);
				return (-1);
			}
			module_opts = S_INIT;
			break;
		case S_I2CTYPE:
			if (!strcasecmp(scalar_value, "bmp85")) {
				cursensor->s_i2ctype = I2C_BMP85;
			}  else if (!strcasecmp(scalar_value, "pcf8591")) {
				cursensor->s_i2ctype = I2C_PCF8591P;
			} else {
				printf("Unsupported i2c type %s\n", scalar_value);
				return (-1);
			}
			module_opts = S_INIT;
			break;
		case S_ADDRESS:
			if (cursensor != NULL) {
				cursensor->s_address = strdup(scalar_value);
			}
			module_opts = S_INIT;
			break;
	}
	return (0);
}


static int yaml_assign_scalar(yaml_event_t *t)
{

	switch(blk_opts) {
		case BLK_MAIN:
			proc_main_opt(t->data.scalar.value);
			break;
		case BLK_SENSORS:
			if (proc_sensors_opt(t->data.scalar.value) < 0)
				return (-1);
			break;
	}


	return (0);
}

/*
 * arg1     - path to config file
 * myconfig - pointer to config structure
 *
 * returns 0 if everything goes good
 */
int 
parse_configfile(const char *path, mqtt_global_cfg_t *myconfig) 
{
	yaml_parser_t	parser;
	yaml_event_t	event;
	FILE		*inputf;
	char		*last_scalar = NULL;
	char		tabbuf[29] = {""}, *p;
	int		ret = 0;

	if ((DEBUG_FLAG & 0xf)) {
		if ((debuglog = fopen("./mqtt_debug.log", "w+")) != NULL) {
			fprintf(debuglog, "===> Debug log start\n");
		} else {
			debuglog = stderr;
		}
	}

	if ((inputf = fopen(path, "r")) == NULL) {
		fprintf(debuglog, "file not found: \"%s\"\n", path);
		return (-1);
	} else {
		dprintf("%s()@ Parsing \"%s\"\n", __func__, path);
	}
	yaml_parser_initialize(&parser);
	yaml_parser_set_input_file(&parser, inputf);
	do {
		if (!yaml_parser_parse(&parser, &event)) {
			dprintf("yaml_parser error!\n");
			/* XXX heap memmory leak */
			return (-1);
		}
		switch (event.type) {
			case YAML_STREAM_START_EVENT: 
#define NEWTAB(x)	strncat(x, "\t", sizeof x)
				NEWTAB(tabbuf);
				dprintf("S-START %d\n", event.data.stream_start.encoding); 
				break;
			case YAML_STREAM_END_EVENT: 
				if ((p = strrchr(tabbuf, '\t')) != NULL)
					*p = '\0';
				dprintf("S-END\n"); 
				break;
			case YAML_ALIAS_EVENT:  
				printf("%sS-anchor %s\n", tabbuf, event.data.alias.anchor); 
				break;
			case YAML_SCALAR_EVENT: 
				dprintf("%sS-scalar %s\n", tabbuf, event.data.scalar.value); 
				if (last_scalar == NULL) {
					last_scalar = strdup(event.data.scalar.value);
				} else {
					free(last_scalar);
					last_scalar = strdup(event.data.scalar.value);
				}
				if (yaml_assign_scalar(&event) != 0) {
					return (-1);
				}
				break;
			case YAML_MAPPING_START_EVENT:  
				dprintf("%sS-MAP START %s:%s@sensors\n", 
						tabbuf,
						event.data.mapping_start.tag, 
						event.data.mapping_start.anchor
						); 

				module_opts = S_INIT;
				
				break;
			case YAML_DOCUMENT_START_EVENT:  
				if (event.data.document_start.version_directive != NULL)
					dprintf("S-DOC START %d.%d\n", 
						event.data.document_start.version_directive->major,  
						event.data.document_start.version_directive->minor
					); 
				break;
			case YAML_DOCUMENT_END_EVENT:  
				dprintf("S-DOC END\n"); 
				break;
			case YAML_SEQUENCE_START_EVENT:  
				NEWTAB(tabbuf);
				dprintf("%sS-SEQ START %s %s\n", 
						tabbuf,
						event.data.sequence_start.anchor,
						last_scalar
						); 
				if (!strncasecmp(last_scalar, "sensor", 6)) {
					dprintf("%s SENSOR CONFIG\n",tabbuf);
					blk_opts = BLK_SENSORS;
				}
				/*  new_block(last_scalar); */
				break;
			case YAML_MAPPING_END_EVENT:
				if (cursensor != NULL) {
					cursensor = NULL;
				}
				dprintf("M-END\n");
				break;
			case YAML_SEQUENCE_END_EVENT:  
				dprintf("%send of SENSOR CONFIG\n", tabbuf);
				dprintf("S-SEQ END\n"); 
				blk_opts = BLK_MAIN;
				break;
			case YAML_NO_EVENT:
				dprintf("null\n"); 
				ret = -1;
				break;
			default:
				dprintf("T=%d\n", event.type);
		}
	} while(event.type != YAML_STREAM_END_EVENT);
	yaml_event_delete(&event);
	yaml_parser_delete(&parser);
	
	myMQTT.sensors = &sensors;
	if (myconfig != NULL) 
		bcopy(&myMQTT, myconfig, sizeof(myMQTT));




	sensor_t *sp;
	sp = sensors.sn_head;
	do {
		fprintf(stderr, "sensor %s  on %s type %d\n", sp->s_name, sp->s_channel, sp->s_type);
		sp = sp->s_next;
	} while (sp != NULL);


	return (ret);
}


