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
		kill `head -1 /var/run/mqtt_rpi.pid`
		#echo 0 > $gpiop
		;;
	*)
		echo "usage: $0 [start|stop]"
		exit 64
esac
