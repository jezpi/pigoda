#!/bin/bash

tput clear
while true;
do
	./graphing.sh all
	./graphing.sh exp
	printf "Last update %s\n" "`date`"
	sleep 300
	tput clear
done
