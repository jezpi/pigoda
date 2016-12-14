/*
 *  $Id: mqtt_rpi.c,v 1.1 2015/05/31 22:41:24 jez Exp jez $
 */
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <signal.h>
#include <unistd.h>

#include <mosquitto.h>
#include <yaml.h>
#include <sqlite3.h>


#include <ncurses.h>

#include "mqtt_db.h"

#include "mqtt.h"
#include "mqtt_cmd.h"

/* Thingspeak */
#ifdef THINGSPEAK
#include "libthingspeak/src/thingspeak.h"
#endif

#ifdef MQTTDEBUG

static unsigned short DEBUG_FLAG=0x4;
/*  /static unsigned short DEBUG_MODE=0x0;*/
#define dprintf if (DEBUG_FLAG) printf
#define ddprintf if (DEBUG_FLAG>0x4) printf
#endif
#ifdef MQTTDEBUG
void trap(void)
{
	int a = 2+2;
	return;
}
#else
#define trap() ;
#endif


struct mq_ch_screen screen_data;



scr_stat_t *
scr_stat_new(struct mq_ch_screen *sd, const char *name, float value)
{
	scr_stat_t 	*sp;

	sp = calloc(1, sizeof(*sp)); /* TODO check if NULL */
	sp->ss_name = strdup(name);
	sp->ss_value = value;	
	time(&sp->ss_lastupdate);

	if (sd->screen_stats_head == NULL) {
		sd->screen_stats_head = sd->screen_stats_tail = sp;
	} else {
		sd->screen_stats_tail->ss_next = sp;
		sd->screen_stats_tail = sp;
	}
	return (sp);
}


scr_stat_t *
scr_stat_find(struct mq_ch_screen *sd, const char *name)
{
	scr_stat_t *sp; 
	scr_stat_t *ret = sp = NULL;

	if ((sp = sd->screen_stats_head) == NULL) {
		return (NULL);
	}
	do {
		if (!strcasecmp(name, sp->ss_name)) {
			ret = sp;
			break;
		}
		sp = sp->ss_next;
	} while (sp != sd->screen_stats_head && sp != NULL);
	return (ret);

}

scr_stat_t *
scr_stat_update(struct mq_ch_screen *sd, const char *name, float value) 
{
	scr_stat_t *sp = NULL;

	if ((sp = sd->screen_stats_head) == NULL) {
		return (NULL);
	}
	do {
		if (!strcasecmp(name, sp->ss_name)) {
			sp->ss_value = value;
			time(&sp->ss_lastupdate);
			return (sp);
		}
		sp = sp->ss_next;
	} while (sp != sd->screen_stats_head && sp != NULL);
	return (sp);
}

topic_t *
topic_add(channel_set_t *chset, char *name)
{
	topic_t *tp;

	tp = calloc(1, sizeof(*tp));
	tp->t_name = strdup(name);

	if (chset->channel_head == NULL) {
		chset->channel_head = chset->channel_tail = tp;
	} else {
		chset->channel_tail->t_next = tp;
		chset->channel_tail = tp;
	}
	return (tp);
}

topic_t *
topic_find(channel_set_t *chset, const char *topic)
{
	topic_t *tp = NULL;
	bool match = false;

	if ((tp = chset->channel_head) == NULL)
		return (NULL);
	do {
		if (mosquitto_topic_matches_sub(tp->t_name, topic, &match) != MOSQ_ERR_SUCCESS)
			return (NULL);
		if (match == true) {
			return (tp);
		}
		tp = tp->t_next;
	} while (tp != NULL && tp != chset->channel_head);
	return (tp);
}



#ifdef THINGSPEAK
struct ts_MQTT {
	ts_context_t *ctx;
} *TSMQTT;
#endif


static mqtt_hnd_t 	 Mosquitto;
mqtt_global_cfg_t	 myMQTT_conf;
static bool 		 proc_command;
static bool 		 verbose_mode;
static volatile bool 	 main_loop;
static sig_atomic_t 	 unknown_signal;
static sig_atomic_t 	 got_SIGUSR1;
static sig_atomic_t 	 got_SIGTERM;
static bool 		 mqtt_conn_dead = false;
extern bool 		 sqlite3_dont_store ;
static FILE 		*logfile;
const char 		*__PROGNAME;
const char 		*__HOSTNAME;
static MQTT_db_t    	*sqlitedb;
static char 		*path_config_file;


static struct mosquitto * MQTT_init(mqtt_hnd_t *, bool, const char *) ;
/* 
 * Most basic routines
 */
static void my_publish_callback(struct mosquitto *, void *, int);
static void my_subscribe_callback(struct mosquitto *, void *, int, int, const int *);
static void my_log_callback(struct mosquitto *, void *, int , const char *);
static void my_message_callback(struct mosquitto *, void *, const struct mosquitto_message *);
static void my_connect_callback(struct mosquitto *, void *, int);
static void register_callbacks(struct mosquitto *);
static int MQTT_loop(void *, int);
static int  MQTT_pub(struct mosquitto *mosq, const char *topic, bool, const char *, ...);
static int MQTT_sub(struct  mosquitto *m, const char *topic_fmt, ...);
int MQTT_printf(const char *, ...);
int MQTT_debug(const char *, ...);
int MQTT_log(const char *, ...);
static void MQTT_finish(mqtt_hnd_t *);

static int MQTT_init_topics(struct mosquitto *, channel_set_t *);

#define NCURSES
#ifdef NCURSES
static WINDOW *init_screen();
void destroy_screen(struct mq_ch_screen *) ;
static int update_screen_stats(struct mq_ch_screen *) ;

#endif /* NCURSES */

static struct mosquitto * MQTT_reconnect(struct mosquitto *m, int *ret);
#ifdef THINGSPEAK
static int MQTT_to_ts(struct ts_MQTT *, MQTT_data_type_t type, float value);
static struct ts_MQTT * MQTT_to_ts_init(const char *apikey, int channel);
#endif /* ! THINGSPEAK */
static bool mqtt_rpi_init(const char *progname, char *conf);
mqtt_cmd_t mqtt_proc_msg(char *, char *);
char * mqtt_poli_proc_msg(char *, char *);
static int set_logging(mqtt_global_cfg_t *myconf);
static void siginfo(int signo, siginfo_t *info, void *context);
static void sig_hnd(int);

static void usage(void);


/*
 * Application used to collect and store information from various MQTT channels
 *
 * usage: mqtt_rpi [config_file]
 */
int
main(int argc, char **argv)
{
	char *bufp;
	struct mosquitto *mosq;
	bool first_run = true;
	int mqloopret=0;
	int ch;

	proc_command = false;
	time(&Mosquitto.mqh_start_time);
	__PROGNAME = basename(argv[0]);
	while ((ch = getopt(argc, argv, "nSvc:")) != -1) {
		switch (ch) {
			case 'S':
				sqlite3_dont_store = true;
				break;
			case 'c':
				path_config_file = strdup(optarg);
				break;
			case 'v':
				verbose_mode = true;
				break;
			case 'n':
				parser_test = true;
				break;
			default:
				usage();
				exit(64);
		}
	}

	if ((main_loop = mqtt_rpi_init(__PROGNAME, path_config_file)) == false) {
		fprintf(stderr, "%s: failed to init\n", __PROGNAME);
		exit(3);
	}

	if (parser_test)
		exit(0);
#ifdef NCURSES
	init_screen(&screen_data);
#endif /* ! NCURSES */
	if ((mosq = MQTT_init(&Mosquitto, false, (myMQTT_conf.identity == NULL?__PROGNAME:myMQTT_conf.identity))) == NULL) {
		fprintf(stderr, "%s: failed to init MQTT protocol \n", __PROGNAME);
		exit(3);

	}

	if (!sqlite3_dont_store)
		sqlitedb = MQTT_initdb(
			(myMQTT_conf.sqlite3_db == NULL?"/var/db/pigoda/sensors.db":myMQTT_conf.sqlite3_db)
		);
	else
		MQTT_log("Not storing to sqlite3 DB\n");

#ifdef THINGSPEAK
	TSMQTT = MQTT_to_ts_init("", );
	/* NOTUSED */
#endif

	MQTT_init_topics(Mosquitto.mqh_mos, myMQTT_conf.mqtt_channels);
	/*MQTT_sub(Mosquitto.mqh_mos, "/environment/#" );*/
	/*MQTT_sub(mosq, "/IoT#");*/

	while (main_loop) {
		/* -1 = 1000ms ,  0 = instant return */
		while((mqloopret = MQTT_loop(mosq, 0)) != MOSQ_ERR_SUCCESS) {
			if (got_SIGUSR1) {
				MQTT_pub(mosq, "/network/broadcast/mqtt_channel/user", false, "user_signal");
				got_SIGUSR1 = 0;
			}
			if (!main_loop) {
				break;
			}
		}

			if (first_run) {
				MQTT_pub(mosq, "/network/broadcast/mqtt_channel", true, "on");
				first_run = false;
			}
		if (mqtt_conn_dead) {
				if (MQTT_reconnect(mosq, NULL) != NULL) {
					MQTT_log("Reconnect success!");
					mqtt_conn_dead = false;
				} else {
					MQTT_log("Reconnect failure! Waiting 5secs");
					sleep(5);
				}
		}
		if (got_SIGTERM) {
			main_loop = false;
			/*fprintf(stderr, "%s) got SIGTERM\n", __PROGNAME);*/
			got_SIGTERM = 0;
			MQTT_pub(mosq, "/network/broadcast/mqtt_channel", true, "off"); /* XXX OBSOLETED*/
		}
		if (proc_command) {
			proc_command = false;
			MQTT_pub(mosq, "/network/mqtt_channel/broadcast", false, "%lu", Mosquitto.mqh_start_time);
		}
#ifdef NCURSES
		update_screen_stats(&screen_data);

		if (screen_data.win != NULL) {
			wrefresh(screen_data.win );
		}

#endif
	  	usleep(155000);
	}
	if (!sqlite3_dont_store)
		MQTT_detachdb(sqlitedb);
	MQTT_printf("End of work");
	MQTT_finish(&Mosquitto);
	fclose(logfile);
	return (0);
}

/*
 * Call of mosquitto_loop used to select(2).
 * it wraps third argument of mosquitto_loop
 * the first argument void *m is casted to struct mosquitto
 * the second argument is a timeout expressed in milliseconds.
 */
static int
MQTT_loop(void *m, int timeout)
{
	struct mosquitto *mos = (struct mosquitto *) m;
	int	ret = 0;

	if (mqtt_conn_dead) {
		return (MOSQ_ERR_SUCCESS);
	}
	ret = mosquitto_loop(mos, timeout, 1);
	if (ret != MOSQ_ERR_SUCCESS) {
		fprintf(logfile, "%s(): %s %d\n", __func__, strerror(errno), ret);
		fflush(logfile);
	}

		switch (ret) {
			case MOSQ_ERR_SUCCESS:
				;
				break;
			case MOSQ_ERR_INVAL:
				MQTT_printf( "Input parametrs invalid\n");
				main_loop = false;
				break;
			case MOSQ_ERR_NOMEM:
				MQTT_printf( "Memory condition\n");
				main_loop = false;
				break;
			case MOSQ_ERR_NO_CONN:
				MQTT_printf( "The client isn't connected to the broker\n");
				main_loop = false;
				break;
			case MOSQ_ERR_CONN_LOST:
				MQTT_printf( "Connection to the broker lost\n");
				mqtt_conn_dead = true;
				break;
			case MOSQ_ERR_PROTOCOL:
				MQTT_printf( "Protocol error in communication with broker\n");
				break;
			case MOSQ_ERR_ERRNO:
				MQTT_printf("System call error errno=%s\n", strerror(errno));
				main_loop = false;
				break;
			default:
				MQTT_printf( "Unknown ret code %d\n", ret);
				main_loop = false;
		}
	return (ret);
}

static void my_publish_callback(struct mosquitto *mosq, void *con, int mid)
{
	MQTT_log( "DEBUG: published mid=%d\n", mid);
	return ;
}

static void
MQTT_finish(mqtt_hnd_t *m)
{
	mosquitto_disconnect(m->mqh_mos);
	mosquitto_destroy(m->mqh_mos);
	mosquitto_lib_cleanup();
#ifdef NCURSES
	destroy_screen(&screen_data);
#endif /* ! NCURSES */
	return;
}

static void
my_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	int i;
	MQTT_log( "DEBUG: Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for(i=1; i<qos_count; i++){
		MQTT_log( ", %d", granted_qos[i]);
	}
	MQTT_log( "\n");
}

char * 
mqtt_poli_proc_msg(char *topic, char *payload)
{
	char	*p;
	char	*pbuf = strdup(payload);
	char	*hnam_buf;
	char	**topics;
	int	topic_cnt, n;
	char 	*ret = NULL;
	enum {ST_BEGIN, ST_LUGAR, ST_ENVIRONMENT, ST_SENSOR, ST_DATA, ST_DONE, ST_ERR} pstate;

	if ((p = strchr(pbuf, '\n')) != NULL)
		*p = '\0';

	if (mosquitto_sub_topic_tokenise(topic, &topics, &topic_cnt) != MOSQ_ERR_SUCCESS) {
		return (NULL);
	}
	pstate = ST_BEGIN;
	for (n=0; topic_cnt >= n && pstate != ST_ERR; n++) {
		switch(n) {
			case 0:
				if (topics[0] == NULL)
					pstate = ST_ENVIRONMENT;
				break;
			case 1:

				if (pstate == ST_ENVIRONMENT && !strcasecmp(topics[n], "environment")) {
					pstate = ST_SENSOR;
				} else
					pstate = ST_ERR;

				break;
			case 2:
				if (pstate == ST_SENSOR ) {
					ret = strdup(topics[n]);
				} 
				break;
		}
	}
	if (topic_cnt >= 2) {
		ret = strdup(topics[topic_cnt - 1]);
	}
	mosquitto_sub_topic_tokens_free(&topics, topic_cnt);
	return (ret);
}

mqtt_cmd_t
mqtt_proc_msg(char *topic, char *payload)
{
	char	*p;
	char	*pbuf = strdup(payload);
	char	*hnam_buf;
	char	**topics;
	int	topic_cnt, n;
	enum {ST_BEGIN, ST_LUGAR, ST_ENVIRONMENT, ST_SENSOR, ST_DATA, ST_DONE, ST_ERR} pstate;
	mqtt_cmd_t curcmd;
	mqtt_cmd_t cmd_hint = ST_ERR;

	if ((p = strchr(pbuf, '\n')) != NULL)
		*p = '\0';

	if (mosquitto_sub_topic_tokenise(topic, &topics, &topic_cnt) != MOSQ_ERR_SUCCESS) {
		return (CMD_ERR);
	}
	float light = -1;
	pstate = ST_BEGIN;
	for (n=0; topic_cnt >= n; n++) {
		switch (n) {
			case 0:
				if (topics[0] == NULL)
					pstate = ST_ENVIRONMENT;
				break;
			case 1:
				if (pstate == ST_ENVIRONMENT && !strcasecmp(topics[n], "environment")) {
					pstate = ST_SENSOR;
				} else
					pstate = ST_ERR;

				break;
			case 2:
				if (pstate == ST_SENSOR && !strcasecmp(topics[n], "light")) {
					cmd_hint = CMD_LIGHT;
					if (topic_cnt > (n+1)) {
						pstate = ST_DATA;
					} else
						pstate = ST_DONE;
				} else if (pstate == ST_SENSOR && !strcasecmp(topics[n], "tempin")) {
					cmd_hint = CMD_TEMPIN;
				} else if (pstate == ST_SENSOR && !strcasecmp(topics[n], "tempout")) {
					cmd_hint = CMD_TEMPOUT;
				} else if (pstate == ST_SENSOR && !strcasecmp(topics[n], "pressure")) {
					cmd_hint = CMD_PRESSURE;
				} else if (pstate == ST_SENSOR && !strcasecmp(topics[n], "humidity")) {
					cmd_hint = CMD_HUMIDITY;
				} else if (pstate == ST_SENSOR && !strcasecmp(topics[n], "pir")) {
					cmd_hint = CMD_PIR;
				} else if (pstate == ST_SENSOR && !strcasecmp(topics[n], "mq-2")) {
					cmd_hint = CMD_MQ2;
				} else {
					curcmd = CMD_ERR;
				}
				break;
			default:
				if (pstate != ST_DATA && topics[n] != NULL)
					pstate = ST_ERR;
				else if (pstate == ST_DATA) {
				}

		}
		if (pstate == ST_ERR) {
			MQTT_log("Cmd parser error on \"%s\":%d\n", topic, topic_cnt);
			break;
		}
	}
	
	if (pstate == ST_DONE && cmd_hint == CMD_ERR) {
		MQTT_printf("DEBUG: data == %s\n", pbuf);

		if (!strncasecmp(pbuf, "STAT", 4)) {
			curcmd = CMD_STATS;
		} else if (!strncasecmp(pbuf, "QUIT", 4)) {
			curcmd = CMD_QUIT;
		} else if (!strncasecmp(pbuf, "STAT_LED", 8)) {
			curcmd = CMD_LED;
		} else {
			curcmd = CMD_ERR;
		}
	} else if (cmd_hint != CMD_ERR) {
		curcmd = cmd_hint;
	}
	mosquitto_sub_topic_tokens_free(&topics, topic_cnt);
	return (curcmd);
}


static void
my_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	/* Pring all log messages regardless of level. */
	MQTT_log(" %d - %s\n", level, str);
	return;
}


static void
my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *msg)
{
	topic_t         *tp;
	mqtt_cmd_t  cmd;
	scr_stat_t 	*sp;
	char *dataf;
	float val;
	time_t curtim;


	if (msg->payloadlen){
		if (( tp = topic_find(myMQTT_conf.mqtt_channels, msg->topic)) != NULL) {
			switch(tp->t_action) {
				case TP_PRINT:
					MQTT_printf("%s = %s\n", msg->topic, msg->payload);
					break;
				case TP_STORE:
		if ((dataf = mqtt_poli_proc_msg(msg->topic, msg->payload)) != NULL) {
			val = atof((char *)msg->payload);
			if ((sp = scr_stat_find(&screen_data, dataf)) == NULL) {
				sp = scr_stat_new(&screen_data, dataf, val);
				if (!sqlite3_dont_store)
					MQTT_createtable(sqlitedb, dataf);
			} else {
				sp->ss_value = val;
			}
			curtim = time(NULL);
			if (sp->ss_lastupdate == curtim) {
				MQTT_debug("Not updating. Duplicated data on %s\n", sp->ss_name);
			} else {
				sp->ss_lastupdate = curtim;
				if (!sqlite3_dont_store) {
					if (MQTT_poli_store(sqlitedb, dataf, val) != 0) {
						MQTT_debug("Store failure %s %s %f\n", msg->topic, msg->payload, val);
						sp->ss_err_sqlite++;
					} else {
						sp->ss_store_sqlite++;
					}
				}
			}
		}

					break;
				case TP_ALERT:
					break;
				case TP_SKIP:
					break;
			} /* switch */
		} else {
			MQTT_debug("unknown message %s = %s\n", msg->topic, msg->payload);
		}
	} else {
		MQTT_printf( "Empty message on %s\n", msg->topic);
	}

	return;
}

static void
my_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	int i;

	if (!result){
		/* Subscribe to broker information topics on successful connect. */
		/*  /mosquitto_subscribe(mosq, NULL, "/#", 0);*/
		/*mosquitto_subscribe(mosq, NULL, "/network/stations", 0);*/
		MQTT_log("Connected sucessfully %s as %s\n", 
				myMQTT_conf.mqtt_host, 
				myMQTT_conf.mqtt_user);
		MQTT_printf("Connected sucessfully to %s as \"%s\":%s. %s %s\n", 
				myMQTT_conf.mqtt_host, 
				myMQTT_conf.mqtt_user,
				myMQTT_conf.identity,
				(sqlite3_dont_store?" Not collecting to DB!":"DB file:"),
				((sqlite3_dont_store == false)?myMQTT_conf.sqlite3_db:"")
				);
	} else {
		switch (result) {
			case CONNACK_REFUSED_PROTOCOL_VERSION:
				MQTT_log("CONNACK failure. Protocol version was refused.\n");
				break;
			case CONNACK_REFUSED_IDENTIFIER_REJECTED:
				MQTT_log("CONNACK failure. Idnetifier has been rejected.\n");
				break;
			case CONNACK_REFUSED_SERVER_UNAVAILABLE:
				MQTT_log("CONNACK failure. Server unavailable.\n");
				break;
			case CONNACK_REFUSED_BAD_USERNAME_PASSWORD:
				MQTT_log("CONNACK failure. username or password incorrect.\n");

				break;
			case CONNACK_REFUSED_NOT_AUTHORIZED:
				MQTT_log("CONNACK failure. Not authorized.\n");
				break;
			default:
				MQTT_log("Unknown CONNACK error!\n");
		}
	}
	return;
}

static struct mosquitto *
MQTT_init(mqtt_hnd_t *m, bool c_sess, const char *id)
{
	int lv_minor;
	int lv_major;
	int lv_rev;

	mosquitto_lib_init();
	mosquitto_lib_version(&lv_major, &lv_minor, &lv_rev);
	MQTT_log( "%s@%s libmosquitto %d.%d rev=%d\n", id, __HOSTNAME, lv_major, lv_minor, lv_rev);

	strncpy(m->mqh_id, id, sizeof(m->mqh_id));
	bzero(m->mqh_msgbuf, sizeof(m->mqh_msgbuf));
	m->mqh_clean_session = c_sess;
	if ((m->mqh_mos = mosquitto_new(id, c_sess, NULL)) == NULL)
		return (NULL);

		/*  XXX */
	mosquitto_username_pw_set(m->mqh_mos, myMQTT_conf.mqtt_user, myMQTT_conf.mqtt_password);

	register_callbacks(m->mqh_mos);
	if (mosquitto_connect(m->mqh_mos, myMQTT_conf.mqtt_host, myMQTT_conf.mqtt_port, 60) != MOSQ_ERR_SUCCESS)
		return (NULL);
	return (m->mqh_mos);
}

static void
register_callbacks(struct mosquitto *mosq)
{
	mosquitto_message_callback_set(mosq, my_message_callback);
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_publish_callback_set(mosq, my_publish_callback);
        mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
        mosquitto_log_callback_set(mosq, my_log_callback);
	return;
}

static int
MQTT_pub(struct mosquitto *mosq, const char *topic, bool perm, const char *fmt, ...)
{
	size_t msglen;
	va_list lst;
	char	msgbuf[BUFSIZ];
	int	mid = 0;
	int	ret;

	if (mqtt_conn_dead) return (0);/* Connection lost MOSQ_ERR_CONN|MOSQ_ERR_CONN_LOST */
	va_start(lst, fmt);
	vsnprintf(msgbuf, sizeof msgbuf, fmt, lst);
	va_end(lst);
	msglen = strlen(msgbuf);
	/*mosquitto_publish(mosq, NULL, "/network/broadcast", 3, Mosquitto.mqh_msgbuf, 0, false);*/
	if (mosquitto_publish(mosq, &mid, topic, msglen, msgbuf, 0, perm) == MOSQ_ERR_SUCCESS) {
		ret = mid;
	} else
		ret = -1;
	return (ret);
}

static int
MQTT_sub(struct mosquitto *m, const char *fmt_topic, ...)
{
	int msgid = 0;
	int ret;
	va_list lst;
	char	msgbuf[BUFSIZ];

	va_start(lst, fmt_topic);
	vsnprintf(msgbuf, sizeof msgbuf, fmt_topic, lst);
	va_end(lst);

	if ((ret = mosquitto_subscribe(m, &msgid, msgbuf, 0)) == MOSQ_ERR_SUCCESS)
		ret = msgid;
	else
		ret = -1;
	return (ret);

}

static struct mosquitto *
MQTT_reconnect(struct mosquitto *m, int *ret)
{
	int	mosq_ret;

	MQTT_log("Reconnecting");
	if ((mosq_ret = mosquitto_reconnect(m)) != MOSQ_ERR_SUCCESS) {
		if (ret != NULL) *ret = mosq_ret;
		return (NULL);
	}
	return (m);
}



#ifdef THINGSPEAK
static struct ts_MQTT *
MQTT_to_ts_init(const char *apikey, int channel)
{
	struct ts_MQTT *tsm;

	tsm = malloc(sizeof(struct ts_MQTT));
	tsm->ctx = ts_create_context(apikey, channel);
	return (tsm);

}

static int 
MQTT_to_ts(struct ts_MQTT *tsm, MQTT_data_type_t type, float value)
{
	ts_datapoint_t data;
	
	if (type == T_TEMPOUT) {
		ts_set_value_f32(&data, value);
		ts_datastream_update(tsm->ctx, 0, "field1", &data);
	} else if (type == T_PRESSURE) {
		ts_set_value_f32(&data, value);
		ts_datastream_update(tsm->ctx, 0, "field4", &data);
	} else if (type == T_TEMPIN) {
		ts_set_value_f32(&data, value);
		ts_datastream_update(tsm->ctx, 0, "field1", &data);
	} else if (type == T_TEMPOUT) {
		ts_set_value_f32(&data, (int)value);
		ts_datastream_update(tsm->ctx, 0, "field3", &data);

	} else if (type == T_LIGHT) {
		ts_set_value_i32(&data, (int)value);
		ts_datastream_update(tsm->ctx, 0, "field5", &data);
	} else {
		/* TODO unknown type error reporting */
		return (-1);
	}
	return (0);
}
#endif /* ! THINGSPEAK */

static bool
mqtt_rpi_init(const char *progname, char *conf)
{
	bool   ret = true;
	char  *configfile, *bufp, *p;
	struct sigaction sa;

	bufp = malloc(HOST_NAME_MAX);
	if (gethostname(bufp, HOST_NAME_MAX) != -1) {
		__HOSTNAME = bufp;
	} else
		__HOSTNAME = "nil";

	/*p = basename(progname);*/
	configfile = ((conf != NULL)?conf:DEFAULT_CONFIG_FILE);
	if (parse_configfile(configfile, &myMQTT_conf) < 0) {
		ret = false;
		fprintf(stderr, "Failed to parse config file \"%s\"\n", configfile);
		return (false);
	}


	sa.sa_handler = sig_hnd;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART ;
	signal(SIGINT, sig_hnd);
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		ret = false;
		printf("%s@sigaction() \"%s\"\n", __PROGNAME, configfile);
	}
	signal(SIGQUIT, sig_hnd);
	signal(SIGUSR1, sig_hnd);
	signal(SIGTERM, SIG_DFL);
	signal(SIGHUP, sig_hnd);
	/*  /sa.sa_handler = sig_hnd;*/
	sa.sa_sigaction = siginfo;
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGQUIT, &sa, NULL) == -1) {
		ret = false;
		printf("%s@sigaction() \"%s\"\n", __PROGNAME, configfile);
	}

	if (set_logging(&myMQTT_conf) < 0)
		ret = false;
	return (ret);
}
/* 
 * Subscribe to previously configured topics
 */
static int 
MQTT_init_topics(struct mosquitto *m, channel_set_t *chset)
{
	topic_t *tp ;
	if ((tp = chset->channel_head) == NULL)
		return (-1);


	do {
		MQTT_sub(m, tp->t_name);
		tp = tp->t_next;
	} while (tp != NULL && tp != chset->channel_head);
	return (0);
}

#ifdef NCURSES
static WINDOW *
init_screen(struct mq_ch_screen *scr) {
	WINDOW *local_win;
	initscr();
	cbreak();

	local_win = newwin(20, 0, 0, 0);
	scrollok(local_win, TRUE);
	wrefresh(local_win);
	scr->win = local_win;
	scr->win_stat = newwin(20, 0, 20, 0);
	wrefresh(scr->win_stat);


	return (local_win);
}

static int
update_screen_stats(struct mq_ch_screen *scr) 
{
	scr_stat_t 	*sp;
	int 			linecnt = 1;
	time_t 		curtime;

	mvwprintw(scr->win_stat, 0, 0, "------------- Live Stats ------>\n");

	if ((sp = scr->screen_stats_head) == NULL) {
		mvwprintw(scr->win_stat, 1, 1, "              No Stats           \n");
		return (-1);
	}
	time(&curtime);
	mvwprintw(scr->win_stat, linecnt++, 1, "              %25s: %11s     fail/ok\tlastupdate", "sensor name", "value");
	do {
		mvwprintw(scr->win_stat, linecnt, 1, "              %25s: %11f     %4lu/%lu\t%9.lu", 
								sp->ss_name, 
								sp->ss_value, 
								sp->ss_err_sqlite,
								sp->ss_store_sqlite,
								(curtime-sp->ss_lastupdate)
			 );
		/* some ordering stuff */
		linecnt++;
		sp = sp->ss_next;
	} while (sp != scr->screen_stats_head && sp != NULL);
	wrefresh(scr->win_stat);
	return (linecnt);
}

void
destroy_screen(struct mq_ch_screen *scr) {
	wrefresh(scr->win);
	delwin(scr->win);
	endwin();
}
#endif /* ! NCURSES */


static int
set_logging(mqtt_global_cfg_t *myconf)
{
	int ret = 0;

	if (myconf->logfile == NULL)
		myconf->logfile = "stderr";

	if (strcasecmp(myconf->logfile, "stderr") == 0) {
		logfile = stderr;
	} else {
		if ((logfile = fopen(myconf->logfile, "a+")) == NULL)  {
			ret = -1;
			fprintf(stderr, "failed to open logfile \"%s\"\n", myconf->logfile);
		}
	}
	return (ret);

}

static void
sig_hnd(int sig)
{
	switch (sig) {
		case SIGTERM:
		case SIGINT:
		case SIGHUP:
		case SIGQUIT:
			got_SIGTERM = 1;
			break;
		case SIGUSR1:
			got_SIGUSR1 = 1;
			break;
		default:
			fprintf(stderr, "got unknown signal %s\n", strsignal(sig));
			fflush(stderr);
			unknown_signal = true;
	}
	return;
}

static void
siginfo(int signo, siginfo_t *info, void *context)
{
	fprintf(stdout, "%d %s %s\n", signo, context, strsignal(signo));
	fflush(stdout);
	main_loop = false;
	return ;
}

/* Log message into the log file */
int
MQTT_log(const char *fmt, ...)
{
	va_list vargs;
	int	ret;
	char	*p;
	char	pbuf[BUFSIZ], timebuf[BUFSIZ];
	time_t	curtime;
	struct tm *tmp;



	time(&curtime);
	tmp = localtime(&curtime);
	strftime(timebuf, sizeof timebuf, "%H:%M:%S %d-%m-%y %z", tmp);
	va_start(vargs, fmt);
	ret = vsnprintf(pbuf, sizeof pbuf, fmt, vargs);
	va_end(vargs);
	if ((p = strrchr(pbuf, '\n')) != NULL) {
		*p = '\0';
	}
	/*
	 * switch (logtype) {
	 * 	case LOG_FILE:
	 * 	...
	 */
	fprintf(logfile, "%s  %s\n", timebuf, pbuf);
	fflush(logfile);
	return (ret);
}


int
MQTT_debug(const char *fmt, ...)
{
	va_list vargs;
	int 	ret;
	size_t	fmtlen;
	char	*fmtbuf, *p;
	char	pbuf[BUFSIZ];

	fmtbuf = strdup(fmt);
	fmtlen = strlen(fmtbuf);
	/*
	*(fmtbuf+fmtlen) = '\0';
	fmtlen--;
	*(fmtbuf+fmtlen) = '\0';
	*/

	va_start(vargs, fmt);
	ret = vsnprintf(pbuf, sizeof pbuf, fmt, vargs);
	va_end(vargs);
	if ((p = strrchr(pbuf, '\n')) != NULL) {
		*p = '\0';
	}
	/*printf("%s\n", pbuf);*/
	if (verbose_mode == true)  {
		if (screen_data.win != NULL) {
			wprintw(screen_data.win, "%s\n", pbuf);
			wrefresh(screen_data.win);
		} else
			fprintf(stderr, "%s\n", pbuf);
	}
#ifdef MQTTDEBUG
	fprintf(logfile, "%s\n", pbuf);
	fflush(logfile);
#endif
	return (ret);
}

int
MQTT_printf(const char *fmt, ...)
{
	va_list vargs;
	int	ret;
	size_t	fmtlen;
	char	*fmtbuf, *p;
	char	pbuf[BUFSIZ];

	fmtbuf = strdup(fmt);
	fmtlen = strlen(fmtbuf);
	/*
	*(fmtbuf+fmtlen) = '\0';
	fmtlen--;
	*(fmtbuf+fmtlen) = '\0';
	*/

	va_start(vargs, fmt);
	ret = vsnprintf(pbuf, sizeof pbuf, fmt, vargs);
	va_end(vargs);
	if ((p = strrchr(pbuf, '\n')) != NULL) {
		*p = '\0';
	}
	if (screen_data.win != NULL) {
		wprintw(screen_data.win, "%s\n", pbuf);
		wrefresh(screen_data.win);
	} else {
		fprintf(stderr, "%s\n", pbuf);
	}
#ifdef MQTTDEBUG
	fprintf(logfile, "%s\n", pbuf);
	fflush(logfile);
#endif
	return (ret);
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-Sv] [-c config_file] \n", __PROGNAME);
	exit(64);
}
