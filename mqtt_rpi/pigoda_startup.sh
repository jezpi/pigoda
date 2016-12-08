#!/bin/sh

### BEGIN INIT INFO
# Provides:             pigoda
# Required-Start:    	$local_fs $remote_fs $network $syslog $named
# Required-Stop:     	$local_fs $remote_fs $network $syslog $named
# Default-Start:        2 3 4 5
# Default-Stop:         0 1 6
# Short-Description:    pigoda v1
# Description: 
#  This is a weather/environment statistic daemon that uses MQTT protocol
#  to broadcast statistics as well as listen for incoming commands for
#  interaction with hardware
#  
### END INIT INFO

DAEMON_NAME="mqtt_rpi"
PIDFILE="/var/run/${DAEMON_NAME}.pid"

case $1 in
	start)
		echo "=> Waiting for network"
		sleep 60; # waiting for network
		/usr/bin/${DAEMON_NAME} -c /etc/mqtt_rpi.yaml
		;;
	restart)
		if [ -f ${PIDFILE} ]; then
			kill `head -1 ${PIDFILE}`
			echo "=> Sleeping 10 before starting"
			sleep 10
		else
			echo "=> Daemon $DAEMON_NAME is not running! (PIDfile not found)"
		fi
		echo "=> Running the daemon"
		/usr/bin/${DAEMON_NAME} -c /etc/mqtt_rpi.yaml
		
		;;
	stop)
		if [ -f ${PIDFILE} ]; then
			kill `head -1 ${PIDFILE}`
		else
			echo "Daemon $DAEMON_NAME is not running! (PIDfile not found)"
		fi
		;;
	status)
		if [ -f ${PIDFILE} ]; then
			mqtt_rpi_pid=`head -1 ${PIDFILE}`;
			if kill -0 $mqtt_rpi_pid > /dev/null 2>&1; then
				echo "${DAEMON_NAME} is running with pid $mqtt_rpi_pid"
			else
				echo "Stale pidfile \"$PIDFILE\""
			fi
		else
			echo "PIDfile not found. mqtt_rpi is not working."
			exit 1;
		fi
		;;
	version)
		/usr/bin/${DAEMON_NAME} -V
		;;
	*)
		echo "usage: $0 [start|restart|stop|status|version]"
		exit 64
esac
