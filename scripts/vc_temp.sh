#!/bin/bash


: ${MQTT_USER:=vc_temp}
: ${MQTT_PASSWORD:=''}
: ${MQTT_HOST:='localhost'}
: ${MQTT_PORT:='1883'}

mqtt_pub () {
#	mosquitto_pub -p 1883 -u badacz -P 'GuwajVas4' -h mail.obin.org $*
	mosquitto_pub -p ${MQTT_PORT} -u ${MQTT_USER} -P ${MQTT_PASSWORD} -h ${MQTT_HOST} $*
}

termination () {
	mqtt_pub -t '/network/broadcast/mqtt_vc_temp.sh' -r -m 'off'
	exit 0
}	
trap termination SIGINT
trap termination SIGQUIT
trap termination SIGTERM
mqtt_pub -t '/network/broadcast/mqtt_vc_temp.sh' -r -m 'on'
while /bin/true; 
do
	/opt/vc/bin/vcgencmd measure_temp |cut -c '6-9' | mqtt_pub -t '/environment/vc_temp_badacz' -l
	if [ $? -ne 0 ]; then
		echo "mqtt_pub failure"|logger
	fi
sleep 30
done
