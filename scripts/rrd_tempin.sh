#!/bin/sh 

dt=`date +%s`
offs=0
RRDFILE="tempin.rrd"

#offs='1462653706'
offs=`sqlite3 /home/jez/code/MQTT/graph_channel/sensors.db 'select timestamp from temp_in order by timestamp asc limit 1;'`
if [ -z $offs ]; then
	echo "could not get an offset"
	exit 3
fi
createdb () {
	rm -v ${RRDFILE}
	printf "=> Creating rrd db: %s %d\n" "${RRDFILE}" "$offs"
	date --date='@'"${offs}"
	rrdtool create ${RRDFILE} \
		--step 60 \
		--start $offs  \
		DS:tempin:GAUGE:120:0:24000 \
		RRA:AVERAGE:0.5:1:864000 \
		RRA:AVERAGE:0.5:60:129600 \
		RRA:AVERAGE:0.5:3600:13392 


}


questdb () {
	for tbl in `sqlite3 $1 '.tables'`
	do
		printf "=> %s\n\t" "$tbl"
		sqlite3 $1 ".schema $tbl"
		printf "\t"


	done
}

case $1 in
	"questdb")
		questdb /home/jez/code/MQTT/graph_channel/sensors.db 
	;;
	"create")
		createdb;
	;;
esac
