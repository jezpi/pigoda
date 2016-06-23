#!/bin/sh

### BEGIN INIT INFO
# Provides:             pigoda
# Required-Start:       $remote_fs $syslog
# Required-Stop:        $remote_fs $syslog
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
#/usr/bin/gpio export 27 output
if [ ! -d '/sys/class/gpio/gpio18' ];then
	/usr/bin/gpio export 18 output
fi

#gpiop="/sys/class/gpio/gpio27/value"

case $1 in
	start)
		/usr/bin/mqtt_rpi /etc/mqtt_rpi.yaml
		#echo 1 > $gpiop
		;;
	stop)
		if [ -f ${PIDFILE} ]; then
			kill `head -1 ${PIDFILE}`
		else
			echo "Daemon $DAEMON_NAME is not running! (PIDfile not found)"
		fi
		#echo 0 > $gpiop
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
			echo "pidfile not found. mqtt_rpi is not working"
			exit 1;
		fi
		;;
	*)
		echo "usage: $0 [start|stop|status]"
		exit 64
esac
