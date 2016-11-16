#!/bin/bash

tput clear
while true;
do
	./graphing.sh exp
	./graphing.sh everything
	printf "Last update %s\n" "`date`"
	sleep 300
	tput clear
done
