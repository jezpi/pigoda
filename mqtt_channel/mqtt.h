/*
 * $Id$
 */
#ifndef _MQTT_H_
#define  _MQTT_H_


typedef enum    {TP_PRINT=0x2, TP_STORE=0x4, TP_ALERT=0x8, TP_SKIP=0xf} topic_action_t;
struct topic {
	char 	*t_name;
	topic_action_t t_action;
	struct topic *t_next;
} ;
typedef struct topic topic_t ;

typedef struct {
	topic_t 	*channel_head;
	topic_t 	*channel_tail;
} channel_set_t ;


typedef struct mqtt_global_config_t {
	const char *pidfile;
	const char *logfile;
	const char *channel_prefix;
	const char *sqlite3_db;
	unsigned int debug_level;
	const char *mqtt_host;
	const char *mqtt_user;
	const char *mqtt_password;
	unsigned short mqtt_port;
	char *identity;
	channel_set_t *mqtt_channels;
} mqtt_global_cfg_t;



typedef struct mqtt_hnd {
	struct mosquitto	*mqh_mos;
#define MAX_MSGBUF_SIZ	BUFSIZ
#define MAX_ID_SIZ	BUFSIZ
	char			 mqh_msgbuf[MAX_MSGBUF_SIZ];
	char 			 mqh_id[MAX_ID_SIZ];
	bool 			 mqh_clean_session;
	time_t			 mqh_start_time;
} mqtt_hnd_t;

typedef struct scr_stat {
	char 	*ss_name;
	float 	 ss_value;
	unsigned long ss_err_sqlite;
	unsigned long ss_store_sqlite;
	time_t 	ss_lastupdate;
	struct scr_stat *ss_next;
} scr_stat_t;

struct mq_ch_screen {
	WINDOW *win;
	WINDOW *win_stat;
	scr_stat_t *screen_stats_head;
	scr_stat_t *screen_stats_tail;
	double	pressure;
	double tempin;
	double tempout;
	double vc_temp;
	double humidity;
	double pir;
	int	light;
	unsigned long err_sqlite[9];
	unsigned long store_sqlite[9];
#define STAT_SQLITE_PRESSURE	0
#define STAT_SQLITE_TEMPIN	1
#define STAT_SQLITE_TEMPOUT	2
#define STAT_SQLITE_LIGHT	3
#define STAT_SQLITE_HUMIDITY	4
#define STAT_SQLITE_PIR		5
} ;


#define DEFAULT_CONFIG_FILE	"/etc/mqtt_channel.yaml"
#define HOST_NAME_MAX	64
extern bool parser_test;
extern int parse_configfile(const char *, mqtt_global_cfg_t *);

/* Some missing mqttlib constants related to CONNACK 4th and 5th were not documented */
#define CONNACK_ACCEPTED 0
#define CONNACK_REFUSED_PROTOCOL_VERSION 1
#define CONNACK_REFUSED_IDENTIFIER_REJECTED 2
#define CONNACK_REFUSED_SERVER_UNAVAILABLE 3
#define CONNACK_REFUSED_BAD_USERNAME_PASSWORD 4
#define CONNACK_REFUSED_NOT_AUTHORIZED 5



#endif /*  ! _MQTT_H_ */
