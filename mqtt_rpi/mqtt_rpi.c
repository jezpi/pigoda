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

#include <mosquitto.h> /* MQTT */
#include <yaml.h> /* config file */
#include <sqlite3.h>

#include <bsd/libutil.h> /* pidfile_open(3) etc. */


#include "mqtt.h"
#include "mqtt_sensors.h" 
#include "mqtt_wiringpi.h"
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

typedef struct mqtt_hnd {
	struct mosquitto	*mqh_mos;
#define MAX_MSGBUF_SIZ	BUFSIZ
#define MAX_ID_SIZ	BUFSIZ
	char			 mqh_msgbuf[MAX_MSGBUF_SIZ];
	char 			 mqh_id[MAX_ID_SIZ];
	bool 			 mqh_clean_session;
	time_t			 mqh_start_time;
} mqtt_hnd_t;

#include "mqtt_cmd.h"

static mqtt_hnd_t Mosquitto;
static FILE *logfile;
const char *__PROGNAME;
const char *__HOSTNAME;
mqtt_global_cfg_t	myMQTT_conf;
static bool proc_command;
static bool mqtt_querylog = false;
static bool mqtt_publish_log = false;
static struct pidfh *mr_pidfile;
static volatile bool main_loop;
static sig_atomic_t unknown_signal;
static sig_atomic_t got_SIGUSR1;
static sig_atomic_t got_SIGTERM;


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
static int MQTT_printf(const char *, ...);
static int MQTT_log(const char *, ...);
static void MQTT_finish(mqtt_hnd_t *);

mqtt_cmd_t mqtt_proc_msg(char *, char *);
static int set_logging(mqtt_global_cfg_t *myconf);
static void sig_hnd(int);

static void usage(void);

static int pool_sensors(struct mosquitto *mosq);
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
	pid_t	procpid;

	proc_command = false;
	time(&Mosquitto.mqh_start_time);
	myMQTT_conf.daemon = 0;

	if ((main_loop = mqtt_rpi_init(argv[0], argv[1])) == false) {
		fprintf(stderr, "%s: failed to init\n", __PROGNAME);
		exit(3);
	}
	if (myMQTT_conf.pidfile != NULL) {
		if ((mr_pidfile = pidfile_open(NULL, 0644, &procpid)) == NULL) {
			fprintf(stderr, "mqtt_rpi is already running with pid %d\n", procpid);
			exit(3);
		}
	}
	if (myMQTT_conf.daemon) {
		daemon(1, 0);
	}
	pidfile_write(mr_pidfile);
		
	if ((mosq = MQTT_init(&Mosquitto, false, __PROGNAME)) == NULL) {
		fprintf(stderr, "%s: failed to init MQTT protocol \n", __PROGNAME);
		MQTT_log("%s: failed to init MQTT protocol \n", __PROGNAME);
		exit(3);

	}
	MQTT_log("Sensors init");
	sensors_init(); /* wiringPiSetup() */
	MQTT_log("Led act init");
	startup_led_act();
	MQTT_log("bmp85 init");
	bmp85_init();

	MQTT_log("Subscribe to /guernika/%s/cmd/#", __HOSTNAME);
	MQTT_sub(Mosquitto.mqh_mos, "/guernika/%s/cmd/#", __HOSTNAME);
	MQTT_log("FAN act init");
	startup_fanctl();

	while (main_loop) {
		/* -1 = 1000ms /  0 = instant return */
		while((mqloopret = MQTT_loop(mosq, 0)) != MOSQ_ERR_SUCCESS) {
			if (first_run) {
				MQTT_pub(mosq, "/guernika/network/broadcast/mqtt_rpi", true, "on");
				first_run = false;
			}
			if (got_SIGUSR1) {
				MQTT_pub(mosq, "/guernika/network/broadcast/mqtt_rpi/user", false, "user_signal");
				got_SIGUSR1 = 0;
			}
		}
		
		
		if (got_SIGTERM) {
			main_loop = false;
			fprintf(stderr, "%s) got SIGTERM\n", __PROGNAME);
			got_SIGTERM = 0;
			MQTT_pub(mosq, "/guernika/network/broadcast/mqtt", true, "off");
		}
		if (proc_command) {
			proc_command = false;
			MQTT_pub(mosq, "/guernika/network/broadcast", false, "%lu", Mosquitto.mqh_start_time);
		}
		pool_sensors(mosq);

	  	usleep(1055000);
	}
	MQTT_printf("End of work");
	MQTT_finish(&Mosquitto);
	pidfile_remove(mr_pidfile);
	fclose(logfile);
	fanctl(FAN_OFF, NULL);
	term_led_act();
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
				MQTT_log("error %d\n", errno);
				main_loop = false;
				break;
			case MOSQ_ERR_INVAL:
				MQTT_log( "input parametrs invalid\n");
				main_loop = false;
				break;
			case MOSQ_ERR_NOMEM:
				MQTT_log( "memory condition\n");
				main_loop = false;
				break;
			case MOSQ_ERR_CONN_LOST:
				MQTT_log( "connection lost\n");
				main_loop = false;
				break;
			case MOSQ_ERR_SUCCESS:
				;
				break;
			default:
				MQTT_log( "unknown ret code %d\n", ret);
				main_loop = false;
		}
	return (ret);
}

static void 
my_publish_callback(struct mosquitto *mosq, void *con, int mid)
{
	if (mqtt_publish_log)
		MQTT_log( "DEBUG: published mid=%d\n", mid);
	return ;
}

static void
MQTT_finish(mqtt_hnd_t *m)
{
	mosquitto_disconnect(m->mqh_mos);
	mosquitto_destroy(m->mqh_mos);
	mosquitto_lib_cleanup();
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
	enum {ST_BEGIN, ST_LUGAR, ST_HOST, ST_CMD, ST_CMDARGS, ST_DONE, ST_ERR} pstate;
	mqtt_cmd_t curcmd = CMD_NIL;

	if ((p = strchr(pbuf, '\n')) != NULL)
		*p = '\0';

	if (mosquitto_sub_topic_tokenise(topic, &topics, &topic_cnt) != MOSQ_ERR_SUCCESS) {
		return (CMD_ERR);
	}

	pstate = ST_BEGIN;
	for (n=0; topic_cnt >= n && pstate != ST_DONE; n++) {
		switch (n) {
			case 0:
				if (topics[0] == NULL)
					pstate = ST_LUGAR;
				break;
			case 1:
				if (pstate == ST_LUGAR && !strncasecmp(topics[n], "guernika", 8)) {
					pstate = ST_HOST;
				} else {
					pstate = ST_ERR;
				}
				break;
			case 2:
				if (pstate == ST_HOST && !strcasecmp(topics[n], __HOSTNAME)) {
					pstate = ST_CMD;
				} else
					pstate = ST_ERR;

				break;
			case 3:
				if (pstate == ST_CMD && !strncasecmp(topics[n], "cmd", 3)) {
					if (topic_cnt > (n+1)) {
						pstate = ST_CMDARGS;
					} else
						pstate = ST_DONE;
				} else
					pstate = ST_ERR;
				break;
			case 4:
				if (pstate == ST_CMDARGS && !strncasecmp(topics[n], "FAN", 3)) {
					curcmd = CMD_FAN;
					pstate = ST_DONE;
				} else
					pstate = ST_ERR;
				break;
			default:
				if (pstate != ST_CMDARGS && topics[n] != NULL)
					pstate = ST_ERR;

		}
		if (pstate == ST_ERR) {
			MQTT_log("cmd parser error on \"%s\":%d\n", topic, topic_cnt);
			break;
		}
	}
	if (pstate == ST_DONE && curcmd == CMD_NIL) {

		if (!strncasecmp(pbuf, "STAT", 4)) {
			curcmd = CMD_STATS;
		} else if (!strncasecmp(pbuf, "QUIT", 4)) {
			curcmd = CMD_QUIT;
		} else if (!strncasecmp(pbuf, "FAN", 4)) {
			curcmd = CMD_FAN;
		} else if (!strncasecmp(pbuf, "STAT_LED", 8)) {
			curcmd = CMD_LED;
		} else {
			curcmd = CMD_ERR;
		}
	}
	mosquitto_sub_topic_tokens_free(&topics, topic_cnt);
	return (curcmd);
}


static void
my_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	/* Pring all log messages regardless of level. */
	MQTT_printf(" %d - %s\n", level, str);
	return;
}


static void 
my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *msg)
{
	mqtt_cmd_t  cmd;

	if (msg->payloadlen){
		cmd = mqtt_proc_msg(msg->topic, msg->payload);
		if (cmd == CMD_ERR) 	
			MQTT_log( "ERROR! Unknown command %s@\"%s\" = %d\n",  msg->payload, msg->topic,cmd);
		else
			MQTT_log("\\Command %s@\"%s\" = %d\n",  msg->payload, msg->topic,cmd);
		switch (cmd) {
			case CMD_STATS:
				proc_command = true;
				break;
			case CMD_QUIT:
				main_loop = false;
				break;
			case CMD_FAN:
				if (!strcasecmp(msg->payload, "FANON")) {
					fanctl(FAN_ON, NULL);
					MQTT_log( "DEBUG: Command Fan on %s/%s= %d\n", msg->topic, msg->payload, cmd);
				} else if(!strcasecmp(msg->payload, "FANOFF")) {
					fanctl(FAN_OFF, NULL);
					MQTT_log( "DEBUG: Command fan off %s/%s= %d\n", msg->topic, msg->payload, cmd);
				} else {
					MQTT_log( "ERROR. Unknown command fan %s/%s= %d\n", msg->topic, msg->payload, cmd);
				}
				break;
			
		}
	}else {
		MQTT_log( "ERROR! Empty message on %s\n", msg->topic);
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
	MQTT_log( "%s@%s libmosquitto %d.%d rev=%d\n", id, __HOSTNAME, lv_major, lv_minor, lv_rev);
	strncpy(m->mqh_id, id, sizeof(m->mqh_id));
	bzero(m->mqh_msgbuf, sizeof(m->mqh_msgbuf));
	m->mqh_clean_session = c_sess;
	if ((m->mqh_mos = mosquitto_new(id, c_sess, NULL)) == NULL)
		return (NULL);

		/*  XXX */
	mosquitto_username_pw_set(m->mqh_mos, NULL, NULL);

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
	signal(SIGTERM, sig_hnd);
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



static int 
pool_sensors(struct mosquitto *mosq)
{
	int light;
	float temp_in=0, temp_out = 0;
	double pressure=0;

	light = pcf8591p_ain(0);
	MQTT_pub(mosq, "/guernika/environment/light", true, "%d", light);

	if ((temp_in = get_temperature("28-0000055a8be7")) != -1)
		MQTT_pub(mosq, "/guernika/environment/tempin", true, "%f", temp_in);
	if ((temp_out = get_temperature("28-000005d3355e")) != -1)
		MQTT_pub(mosq, "/guernika/environment/tempout", true, "%f", temp_out);
	pressure=get_pressure();
	MQTT_pub(mosq, "/guernika/environment/pressure", true, "%0.2f", pressure);
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

static int 
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

static int 
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
	printf("%s\n", pbuf);
#ifdef MQTTDEBUG
	/*fprintf(logfile, "%s\n", pbuf);
	fflush(logfile); XXX disabled because it makes no sense*/
#endif
	return (ret);
}
static void
usage(void)
{
	fprintf(stderr, "usage: %s\n", __PROGNAME);
	exit(64);
}

