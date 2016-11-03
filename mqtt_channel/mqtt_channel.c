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
void tramp(void)
{
	int a = 2+2;
	return;
}
#else
#define tramp() ;
#endif

struct router_stats {
	char	*rs_name;
	char	*rs_tx_rate;
	char	*rs_rx_rate;
};


struct MQTT_statistics {
	unsigned long	mqs_msgcnt;
	unsigned long	mqs_msgbytes;
	int	mqs_last_pub_mid;
	int	mqs_last_mid;

} MQTT_stat;


struct mq_ch_screen {
	WINDOW *win;
	WINDOW *win_stat;
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

} screen_data;

typedef struct mqtt_hnd {
	struct mosquitto	*mqh_mos;
#define MAX_MSGBUF_SIZ	BUFSIZ
#define MAX_ID_SIZ	BUFSIZ
	char			 mqh_msgbuf[MAX_MSGBUF_SIZ];
	char 			 mqh_id[MAX_ID_SIZ];
	bool 			 mqh_clean_session;
	time_t			 mqh_start_time;
} mqtt_hnd_t;

struct ts_MQTT {
	ts_context_t *ctx;
} *TSMQTT;

#include "mqtt_cmd.h"

static mqtt_hnd_t Mosquitto;
static FILE *logfile;
const char *__PROGNAME;
const char *__HOSTNAME;
mqtt_global_cfg_t	myMQTT_conf;
static bool proc_command;
static volatile bool main_loop;
static sig_atomic_t unknown_signal;
static sig_atomic_t got_SIGUSR1;
static sig_atomic_t got_SIGTERM;
static MQTT_db_t    *sqlitedb;

static void my_publish_callback(struct mosquitto *, void *, int);
static void my_subscribe_callback(struct mosquitto *, void *, int, int, const int *);
static void my_log_callback(struct mosquitto *, void *, int , const char *);
static void my_message_callback(struct mosquitto *, void *, const struct mosquitto_message *);
static void my_connect_callback(struct mosquitto *, void *, int);
static void register_callbacks(struct mosquitto *);

static struct mosquitto * MQTT_init(mqtt_hnd_t *, bool, const char *) ;
static int MQTT_loop(void *m, int);
static int  MQTT_pub(struct mosquitto *mosq, const char *topic, bool, const char *, ...);
static int MQTT_sub(struct  mosquitto *m, const char *topic_fmt, ...);
int MQTT_printf(const char *, ...);
int MQTT_log(const char *, ...);
static void MQTT_finish(mqtt_hnd_t *);
static WINDOW *init_screen();
#ifdef THINGSPEAK
static int MQTT_to_ts(struct ts_MQTT *, MQTT_data_type_t type, float value);
static struct ts_MQTT * MQTT_to_ts_init(const char *apikey, int channel);
#endif /* ! THINGSPEAK */

void destroy_screen(struct mq_ch_screen *) ;




mqtt_cmd_t mqtt_proc_msg(char *, char *);

static int set_logging(mqtt_global_cfg_t *myconf);
static void sig_hnd(int);

static void usage(void);


static void siginfo(int signo, siginfo_t *info, void *context);
static bool mqtt_rpi_init(const char *progname, char *conf);
/*
 * Application used to collect and store information from various MQTT channels
 *
 */

/*
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

	proc_command = false;
	time(&Mosquitto.mqh_start_time);

	if ((main_loop = mqtt_rpi_init(argv[0], argv[1])) == false) {
		fprintf(stderr, "%s: failed to init\n", __PROGNAME);
		exit(3);
	}


	if ((mosq = MQTT_init(&Mosquitto, false, __PROGNAME)) == NULL) {
		fprintf(stderr, "%s: failed to init MQTT protocol \n", __PROGNAME);
		exit(3);

	}
	sqlitedb = MQTT_initdb("/var/db/pigoda/sensors.db");

	init_screen(&screen_data);
	MQTT_sub(Mosquitto.mqh_mos, "/guernika/environment/#");
	TSMQTT = MQTT_to_ts_init("YLW6C8UWBXKWMEXZ", 10709);

	while (main_loop) {
		/* -1 = 1000ms /  0 = instant return */
		while((mqloopret = MQTT_loop(mosq, 0)) != MOSQ_ERR_SUCCESS) {
			if (first_run) {
				MQTT_pub(mosq, "/guernika/network/broadcast/mqtt_graph", true, "on");
				first_run = false;
			}
			if (got_SIGUSR1) {
				MQTT_pub(mosq, "/guernika/network/broadcast/mqtt_graph/user", false, "user_signal");
				got_SIGUSR1 = 0;
			}
			if (!main_loop) {
				break;
			}
		}

		if (got_SIGTERM) {
			main_loop = false;
			/*fprintf(stderr, "%s) got SIGTERM\n", __PROGNAME);*/
			got_SIGTERM = 0;
			MQTT_pub(mosq, "/guernika/network/broadcast/mqtt_graph", true, "off");
		}
		if (proc_command) {
			proc_command = false;
			MQTT_pub(mosq, "/guernika/network/mqtt_graph/broadcast", false, "%lu", Mosquitto.mqh_start_time);
		}

		update_screen_stats(&screen_data);

		if (screen_data.win != NULL) {
			wrefresh(screen_data.win );
		}
	  	usleep(155000);
	}

	MQTT_detachdb(sqlitedb);
	MQTT_printf("End of work");
	MQTT_finish(&Mosquitto);
	fclose(logfile);
	return (0);
}


static int
MQTT_loop(void *m, int tout)
{
	struct mosquitto *mos = (struct mosquitto *) m;
	int	ret = 0;

	ret = mosquitto_loop(mos, tout, 1);
	if (ret != MOSQ_ERR_SUCCESS) {
		fprintf(logfile, "%s(): %d\n", __func__, errno);
		fflush(logfile);
	}

		switch (ret) {
			case MOSQ_ERR_ERRNO:
				MQTT_printf("error %d\n", errno);
				main_loop = false;
				break;
			case MOSQ_ERR_INVAL:
				MQTT_printf( "input parametrs invalid\n");
				main_loop = false;
				break;
			case MOSQ_ERR_NOMEM:
				MQTT_printf( "memory condition\n");
				main_loop = false;
				break;
			case MOSQ_ERR_CONN_LOST:
				MQTT_printf( "connection lost\n");
				main_loop = false;
				break;
			case MOSQ_ERR_SUCCESS:
				;
				break;
			default:
				MQTT_printf( "unknown ret code %d\n", ret);
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
	destroy_screen(&screen_data);
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
					pstate = ST_LUGAR;
				break;
			case 1:
				if (pstate == ST_LUGAR && !strncasecmp(topics[n], "guernika", 8)) {
					pstate = ST_ENVIRONMENT;
				} else {
					pstate = ST_ERR;
				}
				break;
			case 2:
				if (pstate == ST_ENVIRONMENT && !strcasecmp(topics[n], "environment")) {
					pstate = ST_SENSOR;
				} else
					pstate = ST_ERR;

				break;
			case 3:
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
	mqtt_cmd_t  cmd;
	float light, tempin, tempout, pressure, humidity, pir;

	if (msg->payloadlen){
		cmd = mqtt_proc_msg(msg->topic, msg->payload);
		if (cmd == CMD_ERR)
			/*MQTT_printf( "Command %s = %d\n", msg->topic, msg->payload, cmd);
		else*/
			MQTT_printf( "CMD_ERR: Unknown command %s@\"%s\" = %d\n",  msg->payload, msg->topic,cmd);
		switch (cmd) {
			case CMD_STATS:
				proc_command = true;
				break;
			case CMD_QUIT:
				main_loop = false;
				break;
			case CMD_LIGHT:
				light = atof(msg->payload);
				screen_data.light = (int) light;
				/*MQTT_log("\\storing light value %d %s\n", (int) light, msg->payload);*/
#ifdef THINGSPEAK
				MQTT_to_ts(TSMQTT, T_LIGHT, light);
#endif /* ! THINGSPEAK */
				if (MQTT_store(sqlitedb, T_LIGHT, light) != 0) {
					MQTT_log("Store failure\n");
					screen_data.err_sqlite[STAT_SQLITE_LIGHT]++;
				} else {
					screen_data.store_sqlite[STAT_SQLITE_LIGHT]++;
				}
				break;
			case CMD_TEMPIN:
				tempin = atof(msg->payload);
				screen_data.tempin = tempin;
#ifdef THINGSPEAK
				MQTT_to_ts(TSMQTT, T_TEMPIN, tempin);
#endif /* ! THINGSPEAK */
				/*MQTT_printf("\\storing tempin %f \n", tempin );*/
				if (MQTT_store(sqlitedb, T_TEMPIN, tempin) != 0) {
					MQTT_log("Store failure\n");
					screen_data.err_sqlite[STAT_SQLITE_TEMPIN]++;
				} else {
					screen_data.store_sqlite[STAT_SQLITE_TEMPIN]++;
				}
				break;
			case CMD_TEMPOUT:
				tempout = atof(msg->payload);
				screen_data.tempout = tempout;
				/*MQTT_printf("\\storing tempout %f \n", tempout );*/
#ifdef THINGSPEAK
				MQTT_to_ts(TSMQTT, T_TEMPOUT, tempout);
#endif /* ! THINGSPEAK */
				if (MQTT_store(sqlitedb, T_TEMPOUT, tempout) != 0) {
					MQTT_log("Store failure\n");
					screen_data.err_sqlite[STAT_SQLITE_TEMPOUT]++;
				} else {
					screen_data.store_sqlite[STAT_SQLITE_TEMPOUT]++;
				}
				break;
			case CMD_PRESSURE:
				pressure = atof(msg->payload);
				screen_data.pressure = pressure;
#ifdef THINGSPEAK
				MQTT_to_ts(TSMQTT, T_PRESSURE, pressure);
#endif /* ! THINGSPEAK */
				/*MQTT_printf("\\storing pressure %f \n", pressure );*/
				if (MQTT_store(sqlitedb, T_PRESSURE, pressure) != 0) {
					MQTT_log("Store failure\n");
					screen_data.err_sqlite[STAT_SQLITE_PRESSURE]++;
				} else {
					screen_data.store_sqlite[STAT_SQLITE_PRESSURE]++;
				}
				break;
			case CMD_HUMIDITY:
				humidity = atof(msg->payload);
				screen_data.humidity = humidity;

				if (MQTT_store(sqlitedb, T_HUMIDITY, humidity) != 0) {
					MQTT_log("Store failure\n");
					screen_data.err_sqlite[STAT_SQLITE_HUMIDITY]++;
				} else {
					screen_data.store_sqlite[STAT_SQLITE_HUMIDITY]++;
				}
				break;

			case CMD_PIR:
				pir = atof(msg->payload);
				screen_data.pir = pir;

				if (MQTT_store(sqlitedb, T_PIR, pir) != 0) {
					MQTT_log("Store failure\n");
					screen_data.err_sqlite[STAT_SQLITE_PIR]++;
				} else {
					screen_data.store_sqlite[STAT_SQLITE_PIR]++;
				}
				break;
			case CMD_MQ2:
				pir = atof(msg->payload);
				screen_data.pir = pir;

				if (MQTT_store(sqlitedb, T_MQ2, pir) != 0) {
					MQTT_log("Store failure\n");
					screen_data.err_sqlite[STAT_SQLITE_PIR]++;
				} else {
					screen_data.store_sqlite[STAT_SQLITE_PIR]++;
				}
				break;
		}
	} else {
		MQTT_log( "Empty message on %s\n", msg->topic);
	}

	return;
}

static void
my_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	int i;

	if(!result){
		/* Subscribe to broker information topics on successful connect. */
		MQTT_sub(mosq, "/guernika/IoT#");
		/*  /mosquitto_subscribe(mosq, NULL, "/guernika/#", 0);*/
		/*mosquitto_subscribe(mosq, NULL, "/guernika/network/stations", 0);*/
		MQTT_printf("Connected sucessfully %s\n", myMQTT_conf.mqtt_host);
	} else {
		MQTT_printf("Connect failed to %s\n", myMQTT_conf.mqtt_host);
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
	MQTT_printf( "%s@%s libmosquitto %d.%d rev=%d\n", id, __HOSTNAME, lv_major, lv_minor, lv_rev);
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

	va_start(lst, fmt);
	vsnprintf(msgbuf, sizeof msgbuf, fmt, lst);
	va_end(lst);
	msglen = strlen(msgbuf);
	/*mosquitto_publish(mosq, NULL, "/guernika/network/broadcast", 3, Mosquitto.mqh_msgbuf, 0, false);*/
	if (mosquitto_publish(mosq, &mid, topic, msglen, msgbuf, 0, perm) == MOSQ_ERR_SUCCESS) {
		ret = mid;
		MQTT_stat.mqs_last_pub_mid = mid;
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
		ts_set_value_i32(&data, (int)value);
		ts_datastream_update(tsm->ctx, 0, "field1", &data);
	} else if (type == T_PRESSURE) {
		ts_set_value_i32(&data, (int)value);
		ts_datastream_update(tsm->ctx, 0, "field4", &data);
	} else if (type == T_TEMPIN) {
		ts_set_value_i32(&data, (int)value);
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
	__PROGNAME = basename(progname);
	configfile = ((conf != NULL)?conf:DEFAULT_CONFIG_FILE);
	if (parse_configfile(configfile, &myMQTT_conf) < 0) {
		ret = false;
		printf("Failed to parse config file \"%s\"\n", configfile);
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

int
update_screen_stats(struct mq_ch_screen *scr) {
	mvwprintw(scr->win_stat, 0, 0, "------------- Live Stats ------>\n");
	mvwprintw(scr->win_stat, 1, 1, "              Light: %11d     %lu/%lu", scr->light,scr->store_sqlite[STAT_SQLITE_LIGHT], scr->err_sqlite[STAT_SQLITE_LIGHT]);
	mvwprintw(scr->win_stat, 2, 1, " Temperature inside: %3.8f     %lu/%lu", scr->tempin, scr->store_sqlite[STAT_SQLITE_TEMPIN], scr->err_sqlite[STAT_SQLITE_TEMPIN]);
	mvwprintw(scr->win_stat, 3, 1, "Temperature outside: %3.8f     %lu/%lu", scr->tempout, scr->store_sqlite[STAT_SQLITE_TEMPOUT] ,scr->err_sqlite[STAT_SQLITE_TEMPOUT]);
	mvwprintw(scr->win_stat, 4, 1, "           Pressure: %f     %lu/%lu", scr->pressure, scr->store_sqlite[STAT_SQLITE_PRESSURE], scr->err_sqlite[STAT_SQLITE_PRESSURE]);
	mvwprintw(scr->win_stat, 5, 1, "           Humidity: %f     %lu/%lu", scr->humidity, scr->store_sqlite[STAT_SQLITE_HUMIDITY], scr->err_sqlite[STAT_SQLITE_HUMIDITY]);
	mvwprintw(scr->win_stat, 6, 1, "                PIR: %f     %lu/%lu", scr->pir, scr->store_sqlite[STAT_SQLITE_PIR], scr->err_sqlite[STAT_SQLITE_PIR]);
	wrefresh(scr->win_stat);
}

void
destroy_screen(struct mq_ch_screen *scr) {
	wrefresh(scr->win);
	delwin(scr->win);
	endwin();
}


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
	/*printf("%s\n", pbuf);*/
	if (screen_data.win != NULL) {
		wprintw(screen_data.win, "%s\n", pbuf);
		wrefresh(screen_data.win);
	} else
		fprintf(stderr, "%s\n", pbuf);
#ifdef MQTTDEBUG
	fprintf(logfile, "%s\n", pbuf);
	fflush(logfile);
#endif
	return (ret);
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s\n", __PROGNAME);
	exit(64);
}
