#include <stdio.h>
#include <stdbool.h>
#include <yaml.h>
#include <curses.h>

#include "mqtt_ADT.h"
#include "mqtt.h"
#define DEBUG
#ifdef DEBUG

channel_set_t *chset;


static unsigned short DEBUG_FLAG=0x0;
/*  /static unsigned short DEBUG_MODE=0x0;*/
#define dprintf if (DEBUG_FLAG) printf
#define ddprintf if (DEBUG_FLAG>0x4) printf
#endif

#define NEWTAB(x)	strncat(x, "\t", sizeof x)
static void print_resume(void);

void
TABRM(char *tabp)
{
	char *p;
	if ((p = strrchr(tabp, '\t')) != NULL) *p = '\0';
}

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

mqtt_global_cfg_t myMQTT;
bool parser_test;
static char *curvar;
FILE *debuglog;


char *channels[10];
int chidx;

static enum {
	V_UNKNOWN, V_PIDFILE, V_LOGFILE, V_DEBUG, V_SERVER, 
	V_MQTTUSER, V_MQTTPASS, V_MQTTPORT, V_CHANNEL_PREFIX, 
	V_SQLITE_PATH, V_IDENTITY, V_CHNAME, V_ON_MESSAGE
     } c_opts = V_UNKNOWN;


static enum {BLK_MAIN, BLK_CHANNELS} blk_opt = BLK_MAIN;
static char		tabbuf[29] = {""}, *p;


char *cur_channel_name;
topic_t *cur_topic;

static int 
yaml_assign_scalar(yaml_event_t *t)
{

	char *p;
	if (DEBUG_FLAG&0xff) fprintf(debuglog, "%s\t[%s() : Processing scalar.value=\"%s\"]\n", tabbuf, __func__ , t->data.scalar.value);
	if (!strcasecmp(t->data.scalar.value, "pidfile")) {
		c_opts = V_PIDFILE;
		myMQTT.pidfile = NULL;
	} else if (!strcasecmp(t->data.scalar.value, "debug")) {
		c_opts = V_DEBUG;
	} else if (!strcasecmp(t->data.scalar.value, "mqtt_user")) {
		c_opts = V_MQTTUSER;
	} else if (!strcasecmp(t->data.scalar.value, "logfile")) {
		c_opts = V_LOGFILE;
	} else if (!strcasecmp(t->data.scalar.value, "mqtt_password")) {
		c_opts = V_MQTTPASS;
	} else if (!strcasecmp(t->data.scalar.value, "mqtt_port")) {
		c_opts = V_MQTTPORT;
	} else if (!strcasecmp(t->data.scalar.value, "mqtt_host")) {
		c_opts = V_SERVER;
	} else if (!strcasecmp(t->data.scalar.value, "channel_prefix")) {
		c_opts = V_CHANNEL_PREFIX;
	} else if (!strcasecmp(t->data.scalar.value, "sqlite3_db")) {
		c_opts = V_SQLITE_PATH;
	} else if (!strcasecmp(t->data.scalar.value, "identity")) {
		c_opts = V_IDENTITY;
	} else if (!strcasecmp(t->data.scalar.value, "name")) {
		c_opts = V_CHNAME;
	} else if (!strcasecmp(t->data.scalar.value, "on_message")) {
		c_opts = V_ON_MESSAGE;
	} else {
		
		switch (c_opts) {
			case V_PIDFILE:
				myMQTT.pidfile = strdup(t->data.scalar.value);
				c_opts = V_UNKNOWN;
				break;
			case V_DEBUG:
				myMQTT.debug_level = (unsigned short) atoi(t->data.scalar.value);
				c_opts = V_UNKNOWN;
				break;
			case V_SERVER:
				myMQTT.mqtt_host = strdup(t->data.scalar.value);
				c_opts = V_UNKNOWN;
				break;
			case V_MQTTUSER:
				myMQTT.mqtt_user = strdup(t->data.scalar.value);
				c_opts = V_UNKNOWN;
				break;
			case V_MQTTPORT:
				myMQTT.mqtt_port = atoi(t->data.scalar.value);
				c_opts = V_UNKNOWN;
				break;
			case V_MQTTPASS:
				myMQTT.mqtt_password = strdup(t->data.scalar.value);
				c_opts = V_UNKNOWN;
				break;
			case V_LOGFILE:
				myMQTT.logfile = strdup(t->data.scalar.value);
				c_opts = V_UNKNOWN;
				break;
			case V_CHANNEL_PREFIX:
				myMQTT.channel_prefix = strdup(t->data.scalar.value);
				c_opts = V_UNKNOWN;
				break;
			case V_SQLITE_PATH:
				myMQTT.sqlite3_db = strdup(t->data.scalar.value);
				c_opts = V_UNKNOWN;
				break;
			case V_IDENTITY:
				myMQTT.identity = strdup(t->data.scalar.value);
				c_opts = V_UNKNOWN;
				break;
			case V_CHNAME:
				cur_topic = topic_add(chset, strdup(t->data.scalar.value));
				c_opts = V_UNKNOWN;
				break;
			case V_ON_MESSAGE:
				if (cur_topic == NULL) {
					return (-1);
				}

				if (!strcasecmp(t->data.scalar.value, "print")) {
					cur_topic->t_action = TP_PRINT;
				} else if (!strcasecmp(t->data.scalar.value, "store")) {
					cur_topic->t_action = TP_STORE;
				} else if (!strcasecmp(t->data.scalar.value, "alert")) {
					cur_topic->t_action = TP_ALERT;
				} else if (!strcasecmp(t->data.scalar.value, "skip")) {
					cur_topic->t_action = TP_SKIP;
				} else
					return (-1);
				break;
		}
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
	int		ret = 0;

	if (parser_test)
		DEBUG_FLAG=0xff;
	if ((DEBUG_FLAG & 0xf)) {
		debuglog = stderr;
	} 

	if ((inputf = fopen(path, "r")) == NULL) {
		if (DEBUG_FLAG & 0xf) {
			fprintf(debuglog, "file not found: \"%s\"\n", path);
		}
		return (-1);
	} else {
		dprintf("%s()@ Parsing \"%s\"\n", __func__, path);
	}
	chset = calloc(1, sizeof(channel_set_t));
	yaml_parser_initialize(&parser);
	yaml_parser_set_input_file(&parser, inputf);
	do {
		if (!yaml_parser_parse(&parser, &event)) {
			if (DEBUG_FLAG & 0xf) {
				fprintf(debuglog, "yaml_parser error!\n");
			}
			/* XXX heap memmory leak */
			return (-1);
		}
		switch (event.type) {
			case YAML_STREAM_START_EVENT: 
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
				NEWTAB(tabbuf);
				dprintf("%sS-MAP START %s:%s@%s\n", 
						tabbuf,
						event.data.mapping_start.tag, 
						event.data.mapping_start.anchor,
						last_scalar
						); 

				
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
				if (!strcasecmp(last_scalar, "channels")) {
					dprintf("%s== channel config\n", tabbuf);
					blk_opt = BLK_CHANNELS;
				}
				/*  new_block(last_scalar); */
				break;
			case YAML_MAPPING_END_EVENT:
				dprintf("%sM-END\n", tabbuf);
				TABRM(tabbuf);
				break;
			case YAML_SEQUENCE_END_EVENT:  
				blk_opt = BLK_MAIN;
				dprintf("%sS-SEQ END\n", tabbuf); 
				TABRM(tabbuf);
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
	if (myconfig != NULL) 
		bcopy(&myMQTT, myconfig, sizeof(myMQTT));

	myconfig->mqtt_channels = chset;
	if ((DEBUG_FLAG & 0x8)) 
		print_resume();
	return (ret);
}


static void
print_resume(void)
{
	topic_t *tp;
	int n = 0;
	printf("======================================================\n");
	printf("| Resume\n");
	tp = chset->channel_head;
	do {
		printf("|\t%s %d\n", tp->t_name, tp->t_action);
		tp = tp->t_next;
	} while (tp != NULL);
	printf("|\n======================================================\n");
}
