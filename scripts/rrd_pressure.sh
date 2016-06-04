#!/bin/sh 

dt=`date +%s`
offs=$((dt-864000))
RRDFILE="pressure.rrd"

#offs='1462653706'
offs=`sqlite3 /home/jez/code/MQTT/graph_channel/sensors.db 'select timestamp from pressure order by timestamp asc limit 1;'`
if [ -z $offs ]; then
	echo "could not get an offset"
	exit 3
fi
create_db () {
	rm -v ${RRDFILE}
	printf "=> Creating rrd db: %s\n" "${RRDFILE}"
	date --date='@'"${offs}"
	rrdtool create ${RRDFILE} \
		--step 60 \
		--start $offs  \
		DS:pressure:GAUGE:120:0:24000 \
		RRA:AVERAGE:0.5:1:864000 \
		RRA:AVERAGE:0.5:60:129600 \
		RRA:AVERAGE:0.5:3600:13392 


}


filldb () {
echo "=> Filling rrd db with data"
sqlite3 -separator " " /home/jez/temperature.db 'select tim,val from vc_temp where tim > '$offs';'|awk '{system("rrdupdate temperature.rrd "$1":"$2);printf(".");}'
}


graph_db() {
echo "=> Creating png graph"
rrdtool graph /var/www/pigoda/temperature.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start $offs --end now \
--font='DEFAULT:7:' \
--title="vc_core temperature " \
--watermark="Date `date`" \
--alt-y-grid \
--rigid \
DEF:temp_core=temperature.rrd:pl:MAX \
LINE1:temp_core#FF00FF:"temp(C)" \
GPRINT:temp_core:MAX:"Max\: %5.2lf" 
}
create_db

