/*
 *  $Id: mqtt_rpi.c,v 1.1 2015/05/31 22:41:24 jez Exp jez $
 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>


#include <pthread.h>
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

#include <bsd/libutil.h> /* pidfile_open(3) etc. */
#include <wiringPi.h> /* defs of HIGH and LOW */


#include "mqtt_parser.h"
#include "mqtt.h"
#include "mqtt_sensors.h"
#include "mqtt_wiringpi.h"
#include "mqtt_cmd.h"

#ifdef MQTTDEBUG

static bool mqtt_connected = false;
static unsigned short DEBUG_FLAG=0x4;
/*  /static unsigned short DEBUG_MODE=0x0;*/
#define dprintf if (DEBUG_FLAG) printf
#define ddprintf if (DEBUG_FLAG>0x4) printf
#endif


struct pir_config {
	gpio_t 		 *pir_gpio;
	gpio_t 		 *pir_led_gpio;
	char		*pir_mqtt_topic;
	struct mosquitto *pir_mosq_connection;
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
static bool shutdown_rpi = false;
static sig_atomic_t unknown_signal;
static sig_atomic_t got_SIGUSR1;
static sig_atomic_t got_SIGTERM;
static sig_atomic_t got_SIGCHLD;
static bool failure;
pthread_mutex_t mqtt_connection_mutex;
static struct mosquitto *mqtt_connection;
static char quit_msg_buf[BUFSIZ];

mqtt_cmd_t mqtt_proc_msg(char *, char *);

static void my_publish_callback(struct mosquitto *, void *, int);
static void my_subscribe_callback(struct mosquitto *, void *, int, int, const int *);
static void my_log_callback(struct mosquitto *, void *, int , const char *);
static void my_message_callback(struct mosquitto *, void *, const struct mosquitto_message *);
static void my_connect_callback(struct mosquitto *, void *, int);
static void my_disconnect_callback(struct mosquitto *, void *, int);
static void register_callbacks(struct mosquitto *);


static struct mosquitto * MQTT_init(mqtt_hnd_t *, bool, const char *) ;
static int MQTT_loop(void *m, int);
static int  MQTT_pub(struct mosquitto *mosq, const char *topic, bool, const char *, ...);
static int MQTT_sub(struct  mosquitto *m, const char *topic_fmt, ...);
static struct mosquitto * MQTT_reconnect(struct mosquitto *m, int *ret);
void *mqtt_pir_th_routine(void *);

int MQTT_printf(const char *, ...);
int MQTT_log(const char *, ...);
static void MQTT_finish(mqtt_hnd_t *);


static int set_logging(mqtt_global_cfg_t *myconf);
static void sig_hnd(int);

static void usage(void);
static void version(void);
static void shutdown_linux(const char *);
static int quit_with_reason(const char *fmt, ...);

const char * print_quit_msg();
static int pool_sensors(struct mosquitto *mosq);
int fork_mqtt_pir(struct pir_config *);
static void siginfo(int signo, siginfo_t *info, void *context);
static bool mqtt_rpi_init(const char *, const char *);
static bool enable_pir(mqtt_global_cfg_t *myconf);

/*
 * Application used to collect and store information from various MQTT channels
 *
 */

/*
 *
 * usage: mqtt_rpi -c [config_file] -f -v -V
 */
int
main(int argc, char **argv)
{
	char *bufp, *configfile = NULL;
	int mqloopret=0;
	int flags, opt;
	struct rlimit lim;
	pid_t	procpid;
	bool first_run = true;
	bool do_pool_sensors = true;
	bool foreground_flg = false;

	proc_command = false;
	time(&Mosquitto.mqh_start_time);
	myMQTT_conf.daemon = 0;
	lim.rlim_cur = RLIM_INFINITY;
	lim.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &lim);
	__PROGNAME = basename(argv[0]);

	while((opt = getopt(argc, argv, "c:vVd:f")) != -1) {
		switch(opt) {
			case 'c':
				configfile = strdup(optarg);
				break;
			case 'd':
				break;
			case 'v':
				break;
			case 'f':
				foreground_flg=true;
				break;
			case 'V':
				version();
				exit(0);
				break;
			default:
				usage();
				exit(64);

		}
	}
	argc-=optind;
	argv+=optind;

	if ((main_loop = mqtt_rpi_init(__PROGNAME, configfile)) == false) {
		fprintf(stderr, "%s: failed to init with %s\n", __PROGNAME, configfile);
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
	if (myMQTT_conf.daemon && ! foreground_flg) {
		daemon(1, 0);
		MQTT_log("Daemonizing!\n");
	} else {
		MQTT_log("Staying in foreground!\n");
	}
	pidfile_write(mr_pidfile);
	if (myMQTT_conf.pool_sensors_delay == 0)
		myMQTT_conf.pool_sensors_delay = 1000000;  /* defaults to 1 sec (which equals 1 000 000usecs*/

	

	MQTT_log("Sensors init");
	if (sensors_init(myMQTT_conf.sensors) <= 0) {
		MQTT_log("No sensors configured properly");

	}	/* wiringPiSetup() bmp85() */

	if (gpios_setup(myMQTT_conf.gpios) > 0) {
		MQTT_log("Led act init");
		if (startup_led_act(10, 100) < 0) {
			MQTT_log("startup blink failure. ");
			
		}
		/*  XXX ugly hack with magic number.
				 It has a magic number which is just to wait until
				 the NIC settles up, it takes a while and i preassume
				 that inmediate data acquisition after reboot is not
				 so critical. It is obviously relative and a workaround.
				 */
	}
	if (startup_fanctl() == 0) {
		MQTT_log("Startup of tip120.");
	} else {
		MQTT_log("TIP120 PWM disabled.");
	}

	if ((mqtt_connection = MQTT_init(&Mosquitto, false, (myMQTT_conf.identity == NULL?__PROGNAME:myMQTT_conf.identity))) == NULL) {
		fprintf(stderr, "%s: failed to init MQTT protocol \n", __PROGNAME);
		MQTT_log("%s: failed to init MQTT protocol \n", __PROGNAME);

		pidfile_remove(mr_pidfile);
		fclose(logfile);
		fanctl(FAN_OFF, NULL);
		term_led_act(true);/* value true indicates failure */
		exit(3);

	}
	if (enable_pir(&myMQTT_conf) == false) {
		MQTT_log("Pir not enabled!");
	} else 
		MQTT_log("Pir activated!");
	
	MQTT_log("Subscribe to /%s/cmd/#", __HOSTNAME);
	MQTT_sub(Mosquitto.mqh_mos, "/%s/cmd/#", __HOSTNAME);
	
	/*
	MQTT_log("FAN act init");
	*/

	while (main_loop) {
		/* -1 = 1000ms /  0 = instant return */
		while((mqloopret = MQTT_loop(mqtt_connection, 0)) != MOSQ_ERR_SUCCESS) {
			if (mqloopret == MOSQ_ERR_CONN_LOST || mqloopret == MOSQ_ERR_NO_CONN) {
				term_led_act(true);/* value true indicates failure */
				break;
			} else if (mqloopret != MOSQ_ERR_SUCCESS) {
				do_pool_sensors = false;
				quit_with_reason("return from MQTT_loop with unrecoverable failure state %d", mqloopret);
				failure = true;
				break;
			}
		}
		if (first_run && mqtt_connected) {
			MQTT_pub(mqtt_connection, "/network/broadcast/mqtt_rpi", true, "on");
			first_run = false;
		}
		if (got_SIGUSR1) {
			MQTT_pub(mqtt_connection, "/network/broadcast/mqtt_rpi/user", false, "user_signal");
			got_SIGUSR1 = 0;
		}
		if (got_SIGCHLD) {
			waitpid(Mosquitto.mqh_mqttpir_pid, NULL, 0);
			MQTT_log("got sigchld - mqttpir terminated!");
			got_SIGCHLD = 0;
		}

		if (mqtt_conn_dead) {
			if (MQTT_reconnect(mqtt_connection, NULL) != NULL) {
				MQTT_log("Reconnect success!");
				mqtt_conn_dead = false;
				do_pool_sensors = true;
				startup_led_act(10, 10); /*  XXX ugly hack*/
			} else {
				MQTT_log("Reconnect failure! Waiting 5secs");
				term_led_act(true);/* value true indicates failure */
				sleep(5);
			}
		}
		if (got_SIGTERM) {
			quit_with_reason("Got SIGTERM");
			MQTT_printf("%s) got SIGTERM\n", __PROGNAME);
			got_SIGTERM = 0;
			MQTT_pub(mqtt_connection, "/network/broadcast/mqtt_rpi", true, "off");
		}
		if (proc_command) {
			proc_command = false;
			MQTT_pub(mqtt_connection, "/network/broadcast/start_time", false, "%lu", Mosquitto.mqh_start_time);
		}
		if (do_pool_sensors) {
			flash_led(NOTIFY_LED, HIGH);
			if (pool_sensors(mqtt_connection) == -1) {
				flash_led(FAILURE_LED, HIGH);
				do_pool_sensors = false;
				MQTT_log("failed to pool sensors. Pooling disabled\n");
			}
			flash_led(NOTIFY_LED, LOW);
		}
		if (poll_pwr_btn() > 0) {
			flash_led(FAILURE_LED, HIGH);
			quit_with_reason("power button pushed!");
			shutdown_rpi = true;
			MQTT_printf("Shutdown\n");
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
	fanctl(FAN_OFF, NULL);
	term_led_act(failure);
	MQTT_log("%sQuit reason: %s\n", ((failure==true)?"(FAILURE) ":""), print_quit_msg());
	if (shutdown_rpi) {
		MQTT_log("Calling shutdown of the machine");
		fflush(logfile);
		shutdown_linux("/sbin/halt");
	}
	fclose(logfile);
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
		MQTT_log("%s(): %d\n", __func__, errno);
	}

		switch (ret) {
			case MOSQ_ERR_ERRNO:

				if (errno == EHOSTUNREACH || errno == ENETUNREACH || errno == ENETRESET || errno == ENETDOWN || errno == ENOTCONN) {
					mqtt_conn_dead = true;/* XXX temporal */
					main_loop = true;
					errno = 0;
					MQTT_log("Connection lost (%s). Reconnecting in a while...\n", strerror(errno));
				} else {
					MQTT_log("System error (errno=%d)=%s\n", errno, strerror(errno));
					quit_with_reason("System error (errno=%d)=%s\n", errno, strerror(errno));
				}
				break;
			case MOSQ_ERR_INVAL:
				MQTT_log( "input parametrs invalid\n");
				quit_with_reason("mosquitto_loop failure = %d", ret);
				break;
			case MOSQ_ERR_NOMEM:
				MQTT_log( "Memory condition\n");
				quit_with_reason("mosquitto_loop failure = %d", ret);
				break;
			case MOSQ_ERR_CONN_LOST:
				MQTT_printf("MQTT_loop: Connection lost (%d)\n", errno);
				MQTT_log( "MQTT_loop: Connection lost (%d)\n", errno);
				mqtt_conn_dead = true;
				break;
			case MOSQ_ERR_NO_CONN:
				MQTT_printf("No connection (%d)\n", errno);
				MQTT_log( "No connection lost\n");
				mqtt_conn_dead = true;
			  break;
			case MOSQ_ERR_SUCCESS:
				;
				break;
			case MOSQ_ERR_AUTH:
				MQTT_log( "Authentication error\n");
				quit_with_reason("mosquitto_loop failure = %d", ret);
				break;
			case MOSQ_ERR_ACL_DENIED:
				MQTT_log( "ACL denied\n");
				quit_with_reason("mosquitto_loop failure = %d", ret);
				break;
			case MOSQ_ERR_CONN_REFUSED:
				MQTT_log( "MQTT_loop: Connection refused\n");
				MQTT_printf( "MQTT_loop: Connection refused\n");
				mqtt_conn_dead = true;
				break;
			default:
				MQTT_log( "unknown ret code %d\n", ret);
				quit_with_reason("mosquitto_loop failure = %d", ret);
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
	MQTT_printf("DEBUG: subscrited (mid: %d): %d\n", mid, granted_qos[0]);
	MQTT_log( "DEBUG: Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for(i=1; i<qos_count; i++){
		MQTT_log( ", %d", granted_qos[i]);
	}
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
				} else if (pstate == ST_CMDARGS && !strncasecmp(topics[n], "RELAY", 5)) {
					curcmd = CMD_RELAY;
					pstate = ST_DONE;
				} else if (pstate == ST_CMDARGS && !strncasecmp(topics[n], "HALT", 4)) {
					curcmd = CMD_HALT;
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
		} else if (!strncasecmp(pbuf, "HALT", 4)) {
			curcmd = CMD_HALT;
		} else if (!strncasecmp(pbuf, "FAN", 3)) {
			curcmd = CMD_FAN;
		} else if (!strncasecmp(pbuf, "RELAY", 5)) {
			curcmd = CMD_RELAY;
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
	if (level != 16) {
		MQTT_log(" %d - %s\n", level, str);
	}
	return;
}


static void
my_disconnect_callback(struct mosquitto *mosq, void *userdata, int rc)
{
	if (rc != 0) {
		MQTT_log("Client disconnected unexpectly rc=%d\n", rc);
		mqtt_conn_dead = true;
	}
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
			case CMD_HALT:
				shutdown_rpi = true;
				quit_with_reason("Received HALT command via MQTT");
				MQTT_printf("Shutdown triggered via MQTT\n");
				break;
			case CMD_QUIT:
				quit_with_reason("Received QUIT command via MQTT");
				break;
			case CMD_FAN:
				if (!strcasecmp(msg->payload, "on")) {
					fanctl(FAN_ON, NULL);
					MQTT_log( "DEBUG: Command Fan on %s/%s= %d\n", msg->topic, msg->payload, cmd);
				} else if(!strcasecmp(msg->payload, "off")) {
					fanctl(FAN_OFF, NULL);
					MQTT_log( "DEBUG: Command fan off %s/%s= %d\n", msg->topic, msg->payload, cmd);
				} else {
					MQTT_log( "ERROR. Unknown command fan %s/%s= %d\n", msg->topic, msg->payload, cmd);
				}
				break;
			case CMD_RELAY:
				if (!strcasecmp(msg->payload, "on")) {
					relay_ctl(1);
					MQTT_log( "DEBUG: Command relay on %s/%s= %d\n", msg->topic, msg->payload, cmd);
				}  else if(!strcasecmp(msg->payload, "off")) {
					relay_ctl(0);
					MQTT_log( "DEBUG: Command relay off %s/%s= %d\n", msg->topic, msg->payload, cmd);
				} else {
					MQTT_log( "ERROR. Unknown relay command %s/%s= %d\n", msg->topic, msg->payload, cmd);
				}
				break;
			default:
				MQTT_log("Command not handled. Internal error!");
 
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
	MQTT_log( "Init %s %s@%s libmosquitto %d.%dr%d\n", __PROGNAME, id, __HOSTNAME, lv_major, lv_minor, lv_rev);
	MQTT_log( "Compiled on %s %s\n", __DATE__, __TIME__);
	MQTT_printf( "Init\t%s@%s libmosquitto %d.%d rev=%d\n", id, __HOSTNAME, lv_major, lv_minor, lv_rev);
	strncpy(m->mqh_id, id, sizeof(m->mqh_id));
	bzero(m->mqh_msgbuf, sizeof(m->mqh_msgbuf));
	m->mqh_clean_session = c_sess;
	if ((m->mqh_mos = mosquitto_new(id, c_sess, NULL)) == NULL)
		return (NULL);

		/*  XXX */
	mosquitto_username_pw_set(m->mqh_mos, myMQTT_conf.mqtt_user, myMQTT_conf.mqtt_password);

	register_callbacks(m->mqh_mos);

	pthread_mutex_init(&mqtt_connection_mutex, NULL);
	pthread_mutex_lock(&mqtt_connection_mutex);
	if ((mosq_ret = mosquitto_connect(m->mqh_mos, myMQTT_conf.mqtt_host, myMQTT_conf.mqtt_port, myMQTT_conf.mqtt_keepalive)) == MOSQ_ERR_SUCCESS) {
		pthread_mutex_unlock(&mqtt_connection_mutex);
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
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_disconnect_callback_set(mosq, my_disconnect_callback);
        mosquitto_log_callback_set(mosq, my_log_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);
	mosquitto_publish_callback_set(mosq, my_publish_callback);
        mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
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

        if (mqtt_conn_dead || ! mqtt_connected) {
		MQTT_printf("Publish too early. Socket not connected (%s)\n", topic);
		return (0);
	}
	va_start(lst, fmt);
	vsnprintf(msgbuf, sizeof msgbuf, fmt, lst);
	va_end(lst);
	msglen = strlen(msgbuf);
	/*mosquitto_publish(mosq, NULL, "/network/broadcast", 3, Mosquitto.mqh_msgbuf, 0, false);*/
	pthread_mutex_lock(&mqtt_connection_mutex);
	if ((pubret = mosquitto_publish(mosq, &mid, topic, msglen, msgbuf, 0, perm)) == MOSQ_ERR_SUCCESS) {
		ret = mid;
		MQTT_stat.mqs_last_pub_mid = mid;
	} else {
		MQTT_printf("Publish error on %s :%s \n", topic, mosquitto_strerror(pubret));
		ret = -1;
	}
	pthread_mutex_unlock(&mqtt_connection_mutex);
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

	pthread_mutex_lock(&mqtt_connection_mutex);
	if ((ret = mosquitto_subscribe(m, &msgid, msgbuf, 0)) == MOSQ_ERR_SUCCESS)
		ret = msgid;
	else
		ret = -1;
	pthread_mutex_unlock(&mqtt_connection_mutex);
	return (ret);

}


static bool
mqtt_rpi_init(const char *progname, const char *conf)
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
	configfile = (char *) ((conf != NULL)?conf:DEFAULT_CONFIG_FILE);
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
int 
fork_mqtt_pir(struct pir_config *cnf)
{
	pid_t 	chpid;
	pthread_t pir_thread;
	int     wstatus;


#ifdef PTHREAD_PIR
	if (cnf != NULL) {
		pthread_create(&pir_thread, NULL, mqtt_pir_th_routine, cnf);
	} else {
		chpid = -1;
	}
#else
#define MQTT_PIR_PATH     "/usr/bin/mqttpir"
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
#endif
	return (chpid);
}


void *
mqtt_pir_th_routine(void *pir_cnf)
{
	struct pir_config *cnf;
	struct mosquitto *m;
	int tick;
	int positive;
	bool actled = false;
	int ledpin, pirpin, val;
	

	cnf = (struct pir_config *) pir_cnf;
	m = cnf->pir_mosq_connection;
	pirpin = cnf->pir_gpio->g_pin;
	if (cnf->pir_led_gpio != NULL)
		ledpin = cnf->pir_led_gpio->g_pin;
	else 
		ledpin = -1;

	pinMode(pirpin, INPUT);
	if (ledpin > 0) {
		pinMode(ledpin, OUTPUT); /* Pin 0? */
		digitalWrite(ledpin, HIGH);
	}

	for (; ; ) {
		for (tick = 0, positive=0; tick < 60; tick++) {
			if ((val = digitalRead(pirpin)) == HIGH) {
				positive++;
				if (ledpin > 0)
					digitalWrite(ledpin, HIGH);
				actled = true;
			} else if (actled == true) {
				if (ledpin > 0)
					digitalWrite(ledpin, LOW);
			}
			usleep(1000000);
			if (actled == true) {
				if (ledpin > 0)
					digitalWrite(ledpin, LOW);
				actled = false;
			}
		}
		MQTT_pub(m, cnf->pir_mqtt_topic, false, "%f", (float)positive/60);
	}

	return (NULL);
}


/*
 * returns -1 on fail, 0 on success
 */
static int
pool_sensors(struct mosquitto *mosq)
{
	char	*endptr;
	bool     sensor_ret = false;
	int      ret = 0;
	float    temp_in=0, temp_out = 0;
	double   value;
	long     pin;

	sensor_t *sp;
	sp = myMQTT_conf.sensors->sn_head;
	do {
		switch(sp->s_type) {
			case SENS_W1:
				sensor_ret = get_temperature(sp->s_address, &value);
				break;
			case SENS_I2C:
				switch(sp->s_i2ctype) {
					case I2C_PCF8591P:
						value = pcf8591p_ain(sp->s_config);
						break;
					case I2C_BMP85:
						sensor_ret = get_pressure(&value);
						break;
					case I2C_SHT30:
						sensor_ret = get_sht30(&value);
						break;
					default:
						MQTT_log("\"%d\" unknown i2c type\n", sp->s_i2ctype);
				}
				break;
			default:
				MQTT_log("\"%s\" unknown sensor type\n", sp->s_name);

		}
		if (sensor_ret == true ) {
			MQTT_pub(mosq, sp->s_channel, false, "%f", value);
		} else {
			MQTT_log("\"%s\" query failure\n", sp->s_name);
			return(-1);
		}
		sp = sp->s_next;
	} while (sp != myMQTT_conf.sensors->sn_head && sp != NULL);

	return (ret);
}

/*
 *
 * struct pir_config {
 *	gpio_t 		*pir_gpio;
 *	gpio_t 		*pir_led_gpio;
 *	char		*pir_mqtt_topic;
 *	struct mosquitto *pir_mosq_connection;
 * };
 */

static bool 
enable_pir(mqtt_global_cfg_t *myconf)
{
	struct pir_config *pir;

	if (myconf->gpios == NULL)  {
		MQTT_printf("GPIOs not configured\n");
		return (false);
	}
	pir = malloc(sizeof(struct pir_config));

	if ((pir->pir_gpio = gpiopin_by_type(myconf->gpios, G_PIR_SENSOR, NULL)) == NULL) {
		MQTT_log("PIR_SENSOR configuration is missing\n");
		return (false);
	}
	if (pir->pir_gpio->g_topic == NULL) {
		MQTT_log("You must specify a topic to publish in\n");
		return (false);
	}

	pir->pir_mqtt_topic = strdup(pir->pir_gpio->g_topic);
	pir->pir_mosq_connection = mqtt_connection;

	if ((pir->pir_led_gpio = gpiopin_by_type(myconf->gpios, G_LED_FAILURE, NULL)) == NULL) {
		MQTT_log("G_LED_FAILURE (which is used by pir to report activity is not configured\n");
	}
	fork_mqtt_pir(pir);
	return (true);
	
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
	strftime(timebuf, sizeof timebuf, "%H:%M:%S %d-%m ", tmp);
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
	fprintf(logfile, "%s%s\n", timebuf, pbuf);
	fflush(logfile);
	return (ret);
}

const char *
print_quit_msg()
{
	return (quit_msg_buf);
}

static int
quit_with_reason(const char *fmt, ...)
{
	va_list vargs;
	int	ret;
	size_t	fmtlen;
	char	*fmtbuf, *p;
	char	pbuf[BUFSIZ];

	va_start(vargs, fmt);
	ret = vsnprintf(quit_msg_buf, sizeof quit_msg_buf, fmt, vargs);
	va_end(vargs);
	if ((p = strrchr(quit_msg_buf, '\n')) != NULL) {
		*p = '\0';
	}
	main_loop = false;
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
	printf("%s\n", pbuf);
#ifdef MQTTDEBUG
	/*fprintf(logfile, "%s\n", pbuf);
	fflush(logfile); XXX disabled because it makes no sense*/
#endif
	return (ret);
}

static void 
shutdown_linux(const char *halt_path)
{
	switch(fork()) {
		case 0:
			execlp(halt_path, halt_path, "-p", NULL);
			MQTT_printf("execlp error when executing halt (%s) %s\n", halt_path, strerror(errno));
			exit(3);
			break;
		case -1:
			
			break;
		default:
			MQTT_printf("Executed halt command\n");
	}
	return;
}
static void
version(void)
{
	fprintf(stderr, "%s Compiled with gcc %s on %s %s\n", __PROGNAME, __VERSION__, __DATE__, __TIME__);
#ifdef MQTTDEBUG
	fprintf(stderr, "\tMQTTDEBUG\t- enabled\n");
#else
	fprintf(stderr, "\tMQTTDEBUG\t- disabled\n");
#endif
#ifdef PTHREAD_PIR
	fprintf(stderr, "\tPTHREAD_PIR\t- enabled\n");
#else
	fprintf(stderr, "\tPTHREAD_PIR\t- disabled\n");
#endif
#ifdef __OPTIMIZE__
	fprintf(stderr, "\tOptimized\n");
#endif
#ifdef PARSER_DEBUG
	fprintf(stderr, "\tPARSER_DEBUG\t- enabled\n");
#else
	fprintf(stderr, "\tPARSER_DEBUG\t- disabled\n");
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        printf("\tbyte order=LITTLE_ENDIAN\n");
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        printf("\tbyte order=BIG_ENDIAN\n");
#endif
#ifdef __ELF__
	fprintf(stderr, "\telf binary\n");
#endif

}

static void
usage(void)
{
	fprintf(stderr, "usage: %s\n", __PROGNAME);
	exit(64);
}
