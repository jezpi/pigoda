#!/bin/sh 

dt=`date +%s`
offs=$((dt-10800))
rm pir.rrd
echo "=> Creating rrd db"
rrdtool create pir.rrd \
--step 60 \
--start  $offs  \
DS:pir:GAUGE:120:0:100 \
RRA:AVERAGE:0.5:1:1500 \

echo "=> Filling rrd db with data"
sqlite3 -separator " " /home/jez/code/MQTT/graph_channel/sensors.db 'select timestamp,pir from pir order by timestamp;'|awk '{system("rrdupdate pir.rrd "$1":"$2);printf(".");}'

echo "=> Creating png graph"
rrdtool graph /var/www/pigoda/pir.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start 1464595304 --end now \
--font='DEFAULT:7:' \
--title="PIR sensor stats " \
--watermark="Date `date`" \
--alt-y-grid \
--rigid \
DEF:movment=pir.rrd:pir:AVERAGE \
LINE1:movment#0F00FF:"temp(C)" \
GPRINT:movment:MAX:"Max\: %5.2lf" \
GPRINT:movment:LAST:"Max\: %5.2lf" 



