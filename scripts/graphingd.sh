#!/bin/bash

sigint() {
	echo "mqtt_graph = off"
	mosquitto_pub -r -h localhost -u guernika -P umyka -t '/network/broadcast/mqtt_graph' -m 'off' 
	exit 3
}

trap sigint SIGINT
echo "mqtt_graph = on"
mosquitto_pub -r -h localhost -u guernika -P umyka -t '/network/broadcast/mqtt_graph' -m 'on' 
tput clear
while true;
do
	./graphing.sh update
	./graphing.sh daily
	./graphing.sh auto
	./graphing.sh custom
	printf "Last update %s\n" "`date`"
	sleep 300
	tput clear
done
