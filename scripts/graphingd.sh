#!/bin/bash

sigint() {
	echo "SIGINT arr"
	mosquitto_pub -r -h localhost -u guernika -P umyka -t '/network/broadcast/mqtt_graph' -m 'off' 
	exit 3
}

trap sigint SIGINT
mosquitto_pub -r -h localhost -u guernika -P umyka -t '/network/broadcast/mqtt_graph' -m 'on' 
tput clear
while true;
do
	./graphing.sh exp
	./graphing.sh daily
	./graphing.sh everything
	./graphing.sh custom
	printf "Last update %s\n" "`date`"
	sleep 300
	tput clear
done
