/*
 *  $Id: mqtt_rpi.c,v 1.1 2015/05/31 22:41:24 jez Exp jez $
 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

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
#include <wiringPi.h> /* defs of HIGH and LOW */

#include "mqtt.h"
#include "mqtt_sensors.h"
#include "mqtt_wiringpi.h"
#ifdef MQTTDEBUG

static bool mqtt_connected = false;
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
	pid_t 			 mqh_mqttpir_pid;
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
static bool main_loop;
static bool mqtt_conn_dead = false;
static sig_atomic_t unknown_signal;
static sig_atomic_t got_SIGUSR1;
static sig_atomic_t got_SIGTERM;
static sig_atomic_t got_SIGCHLD;
static unsigned short failure;


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
static int fork_mqtt_pir(mqtt_hnd_t *);
static void siginfo(int signo, siginfo_t *info, void *context);
static bool mqtt_rpi_init(const char *progname, char *conf);
static struct mosquitto * MQTT_reconnect(struct mosquitto *m, int *ret);
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
	int mqloopret=0;
	struct rlimit lim;
	pid_t	procpid;
	bool first_run = true;
	bool do_pool_sensors = true;

	proc_command = false;
	time(&Mosquitto.mqh_start_time);
	myMQTT_conf.daemon = 0;
	lim.rlim_cur = RLIM_INFINITY;
	lim.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &lim);

	if ((main_loop = mqtt_rpi_init(argv[0], argv[1])) == false) {
		fprintf(stderr, "%s: failed to init\n", __PROGNAME);
		exit(3);
	}
	if (myMQTT_conf.sensors == NULL) {
		MQTT_printf("No sensors configured");
		exit(3);
	}

	if (myMQTT_conf.pidfile != NULL) {
		if ((mr_pidfile = pidfile_open(myMQTT_conf.pidfile, 0644, &procpid)) == NULL) {
			fprintf(stderr, "mqtt_rpi is already running with pid %d\n", procpid);
			exit(3);
		}
	}
	if (myMQTT_conf.daemon) {
		daemon(1, 0);
		MQTT_log("Daemonizing!\n");
	} else {
		MQTT_log("Staying in foreground!\n");
	}
	pidfile_write(mr_pidfile);
	if (myMQTT_conf.pool_sensors_delay == 0)
		myMQTT_conf.pool_sensors_delay = 1055000;  /* defaults to 1 sec */

	if (fork_mqtt_pir(&Mosquitto) <= 0) {
		MQTT_log("Fork failed!");
		/* ...
		 * exit() 
		 */
	}

	MQTT_log("Sensors init");
	sensors_init(myMQTT_conf.sensors); /* wiringPiSetup() */
	MQTT_log("Led act init");
	startup_led_act(10, 100); /*  XXX ugly hack with magic number.
				 It has a magic number which is just to wait until
				 the NIC settles up, it takes a while and i preassume
				 that inmediate data acquisition after reboot is not
				 so critical. It is obviously relative and a workaround.
				 */
	startup_fanctl();
	if ((mosq = MQTT_init(&Mosquitto, false, __PROGNAME)) == NULL) {
		fprintf(stderr, "%s: failed to init MQTT protocol \n", __PROGNAME);
		MQTT_log("%s: failed to init MQTT protocol \n", __PROGNAME);

		pidfile_remove(mr_pidfile);
		fclose(logfile);
		fanctl(FAN_OFF, NULL);
		term_led_act(1);/* value 1 indicates failure */
		exit(3);

	}
	
	MQTT_log("Subscribe to /%s/cmd/#", __HOSTNAME);
	MQTT_sub(Mosquitto.mqh_mos, "/%s/cmd/#", __HOSTNAME);
	
	/*
	MQTT_log("FAN act init");
	*/

	while (main_loop) {
		/* -1 = 1000ms /  0 = instant return */
		while((mqloopret = MQTT_loop(mosq, 0)) != MOSQ_ERR_SUCCESS) {
			if (mqloopret == MOSQ_ERR_CONN_LOST || mqloopret == MOSQ_ERR_NO_CONN) {
				term_led_act(1);/* value 1 indicates failure */
				break;
			} else if (mqloopret != MOSQ_ERR_SUCCESS) {
					do_pool_sensors = false;
					main_loop = false;
					failure = 1;
					break;
			}
			if (first_run) {
				MQTT_pub(mosq, "/network/broadcast/mqtt_rpi", true, "on");
				first_run = false;
			}
			if (got_SIGUSR1) {
				MQTT_pub(mosq, "/network/broadcast/mqtt_rpi/user", false, "user_signal");
				got_SIGUSR1 = 0;
			}
			if (got_SIGCHLD) {
				waitpid(Mosquitto.mqh_mqttpir_pid, NULL, 0);
				MQTT_log("got sigchld - mqttpir terminated!");
				got_SIGCHLD = 0;
			}
		}

		if (mqtt_conn_dead) {
			if (MQTT_reconnect(mosq, NULL) != NULL) {
				MQTT_log("Reconnect success!");
				mqtt_conn_dead = false;
				do_pool_sensors = true;
				startup_led_act(10, 10); /*  XXX ugly hack*/
			} else {
				MQTT_log("Reconnect failure! Waiting 5secs");
				sleep(5);
			}
		}
		if (got_SIGTERM) {
			main_loop = false;
			fprintf(stderr, "%s) got SIGTERM\n", __PROGNAME);
			got_SIGTERM = 0;
			MQTT_pub(mosq, "/network/broadcast/mqtt", true, "off");
		}
		if (proc_command) {
			proc_command = false;
			MQTT_pub(mosq, "/network/broadcast", false, "%lu", Mosquitto.mqh_start_time);
		}
		if (do_pool_sensors) {
			flash_led(GREEN_LED, HIGH);
			if (pool_sensors(mosq) == -1) {
				flash_led(RED_LED, HIGH);
				do_pool_sensors = false;
				MQTT_log("failed to pool sensors. Pooling disabled\n");
			}
			flash_led(GREEN_LED, LOW);
		}
		
	  	usleep(myMQTT_conf.pool_sensors_delay);
	}

	MQTT_printf("End of work");
	if (Mosquitto.mqh_mqttpir_pid > 0) {
		kill(Mosquitto.mqh_mqttpir_pid, SIGTERM);
		waitpid(Mosquitto.mqh_mqttpir_pid, NULL, 1);
		MQTT_log("mqttpir terminated");
	}
	MQTT_finish(&Mosquitto);
	pidfile_remove(mr_pidfile);
	fclose(logfile);
	fanctl(FAN_OFF, NULL);
	term_led_act(failure);
	return (0);
}

static int
MQTT_loop(void *m, int tout)
{
	struct mosquitto *mos = (struct mosquitto *) m;
	int	ret = 0;

  	if (mqtt_conn_dead) {
		return (MOSQ_ERR_SUCCESS);
	}
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
				MQTT_log( "Memory condition\n");
				main_loop = false;
				break;
			case MOSQ_ERR_CONN_LOST:
				MQTT_log( "Connection lost (%d)\n", errno);
				/*main_loop = false; */
				mqtt_conn_dead = true;
				break;
			case MOSQ_ERR_NO_CONN:
				MQTT_log( "No connection lost\n");
				/*main_loop = false;*/
				mqtt_conn_dead = true;
			  break;
			case MOSQ_ERR_SUCCESS:
				;
				break;
			case MOSQ_ERR_AUTH:
				MQTT_log( "Authentication error\n");
				main_loop = false;
				break;
			case MOSQ_ERR_ACL_DENIED:
				MQTT_log( "ACL denied\n");
				main_loop = false;
				break;
			case MOSQ_ERR_CONN_REFUSED:
				MQTT_log( "Connection refused\n");
				mqtt_conn_dead = true;/* XXX temporal */
				/*main_loop = false;*/
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
					pstate = ST_HOST;
				break;
			case 1:
				if (pstate == ST_HOST && !strcasecmp(topics[n], __HOSTNAME)) {
					pstate = ST_CMD;
				} else
					pstate = ST_ERR;

				break;
			case 2:
				if (pstate == ST_CMD && !strncasecmp(topics[n], "cmd", 3)) {
					if (topic_cnt > (n+1)) {
						pstate = ST_CMDARGS;
					} else
						pstate = ST_DONE;
				} else
					pstate = ST_ERR;
				break;
			case 3:
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
my_connect_callback(struct mosquitto *mosq, void *userdata, int retcode)
{
	int i;

	if(!retcode){
		/* Subscribe to broker information topics on successful connect. */
		MQTT_sub(mosq, "/IoT#");
		/*  /mosquitto_subscribe(mosq, NULL, "/#", 0);*/
		/*mosquitto_subscribe(mosq, NULL, "/network/stations", 0);*/
		MQTT_printf("Connected sucessfully %s\n", myMQTT_conf.mqtt_host);
		mqtt_connected = true;
	} else { 
		mqtt_connected = false;

		switch (retcode) {
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
				MQTT_log("CONNACK failure. username (%s) or password incorrect.\n", myMQTT_conf.mqtt_user);
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
	int mosq_ret;

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

	if ((mosq_ret = mosquitto_connect(m->mqh_mos, myMQTT_conf.mqtt_host, myMQTT_conf.mqtt_port, 600)) == MOSQ_ERR_SUCCESS) {
		return (m->mqh_mos);
	} else {
		MQTT_log( "MQTT connect failure! %d %s\n", mosq_ret, strerror(errno));
	}
	return (NULL);
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
	int	ret, pubret;

        if (mqtt_conn_dead || ! mqtt_connected) return (0);
	va_start(lst, fmt);
	vsnprintf(msgbuf, sizeof msgbuf, fmt, lst);
	va_end(lst);
	msglen = strlen(msgbuf);
	/*mosquitto_publish(mosq, NULL, "/network/broadcast", 3, Mosquitto.mqh_msgbuf, 0, false);*/
	if ((pubret = mosquitto_publish(mosq, &mid, topic, msglen, msgbuf, 0, perm)) == MOSQ_ERR_SUCCESS) {
		ret = mid;
		MQTT_stat.mqs_last_pub_mid = mid;
	} else {
		MQTT_printf("Publish error on %s \n", mosquitto_strerror(pubret));
		ret = -1;
	}
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
		fprintf(stderr, "Failed to parse config file \"%s\"\n", configfile);
		return (false);
	}


	sa.sa_handler = sig_hnd;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART ;
	signal(SIGINT, sig_hnd);
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		ret = false;
		fprintf(stderr, "%s@sigaction() \"%s\"\n", __PROGNAME, configfile);
	}
	signal(SIGQUIT, sig_hnd);
	signal(SIGUSR1, sig_hnd);
	signal(SIGTERM, sig_hnd);
	signal(SIGCHLD, sig_hnd);
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
 * fork()/exec() an instance of mqttpir
 */
static int 
fork_mqtt_pir(mqtt_hnd_t *mqh)
{
	pid_t 	chpid;
	int     wstatus;

#define MQTT_PIR_PATH     "/home/jez/repos/pigoda/mqtt_rpi/mqttpir/mqttpir"
	switch((chpid = fork())) {
		case 0: /* XXX hardcoded path */
			execlp(MQTT_PIR_PATH, MQTT_PIR_PATH, NULL);
			MQTT_log("Failed to exec %s: %s", MQTT_PIR_PATH, strerror(errno));
			exit(0);
			break;
		case -1:
			MQTT_log("Failed to fork/exec mqttpir");
			break;
		default:
			MQTT_log("Forked mqttpir with pid=%d", chpid);
			mqh->mqh_mqttpir_pid = chpid;
	}
	return (chpid);
}


/*
 * returns -1 on fail, 0 on success
 */
static int
pool_sensors(struct mosquitto *mosq)
{
	int ret = 0;
	int light;
	float temp_in=0, temp_out = 0;
	double pressure=0;
	double value;
	char	*endptr;
	long pin;

	sensor_t *sp;
	sp = myMQTT_conf.sensors->sn_head;
	do {
		switch(sp->s_type) {
			case SENS_W1:
				value = get_temperature(sp->s_address);
				break;
			case SENS_I2C:
				switch(sp->s_i2ctype) {
					case I2C_PCF8591P:
						value = pcf8591p_ain(sp->s_config);
						break;
					case I2C_BMP85:
						value = get_pressure();
						break;
				}
				break;

		}
		MQTT_pub(mosq, sp->s_channel, true, "%f", value);

		sp = sp->s_next;
	} while (sp != myMQTT_conf.sensors->sn_head && sp != NULL);

	return (ret);
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
		case SIGCHLD:
			got_SIGCHLD = 1;
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
