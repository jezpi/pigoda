#!/bin/sh

dt=`date +%s`
offs=$((dt-10800))


graph_pir() {
echo "=> Creating png graph - pir"
	rrdtool graph /var/www/pigoda/pir.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start now-12h --end now \
		--font='DEFAULT:7:' \
		--title="PIR sensor stats - movement rate in one second" \
		--watermark="Date `date`" \
		--alt-y-grid \
		--rigid \
		DEF:movm=/var/db/rrdcache/pir.rrd:pir:AVERAGE \
		VDEF:movmlast=movm,AVERAGE \
		AREA:movm#0000FF:"Movement rate\n" \
		LINE1:movmlast#FF00FF:"Average movement rate\n":dashes \
		GPRINT:movm:AVERAGE:"Avg\: %5.2lf\n" 

}

graph_tempin() {
echo "=> Creating png graph - tempin"
	rrdtool graph /var/www/pigoda/tempin.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start $offs --end now \
		--font='DEFAULT:7:' \
		--title="Temperature inside" \
		--watermark="Date `date`" \
		--alt-y-grid \
		--rigid \
		DEF:temp_in=/var/db/rrdcache/tempin.rrd:tempin:AVERAGE \
		VDEF:tempinlast=temp_in,LAST \
		AREA:temp_in#0000FF:"temperature(C)\n" \
		LINE1:tempinlast#FF00FF:"tempemperature last (C)\n":dashes \
		GPRINT:temp_in:MAX:"Max\: %5.2lf\n" \
		GPRINT:temp_in:AVERAGE:"Avg\: %5.2lf\n" \
		GPRINT:temp_in:MIN:"Min\: %5.2lf\n" \
		GPRINT:temp_in:LAST:"Last\: %5.2lf\n" 
}

graph_tempin_weekly() {
echo "=> Creating png graph - tempin"
	rrdtool graph /var/www/pigoda/tempin_weekly.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start 'now-14d' --end now \
		--font='DEFAULT:7:' \
		--title="Temperature inside - past 14 days" \
		--watermark="Date `date`" \
		--alt-y-grid \
		--rigid \
		DEF:temp_in=/var/db/rrdcache/tempin.rrd:tempin:AVERAGE \
		VDEF:tempinlast=temp_in,LAST \
		AREA:temp_in#0000FF:"temp(C)\n" \
		LINE1:tempinlast#FF00FF:"temp(C)\n":dashes \
		GPRINT:temp_in:MAX:"Max\: %5.2lf\n" \
		GPRINT:temp_in:AVERAGE:"Avg\: %5.2lf\n" \
		GPRINT:temp_in:MIN:"Min\: %5.2lf\n" 
}

graph_vc_temp() {
	echo "=> Creating png graph - vc_temp"
	rrdtool graph /var/www/pigoda/temperature.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start $offs --end now \
		--font='DEFAULT:7:' \
		--title="vc_core temperature " \
		--watermark="Date `date`" \
		--alt-y-grid \
		--rigid \
		DEF:vc_temp=/var/db/rrdcache/vc_temp.rrd:vctemp:AVERAGE \
		LINE1:vc_temp#D40000:"vc_temp(C)" \
		GPRINT:vc_temp:MAX:"Max\: %5.2lf" \
		GPRINT:vc_temp:AVERAGE:"Avg\: %5.2lf" \
		GPRINT:vc_temp:MIN:"Min\: %5.2lf" \
		GPRINT:vc_temp:LAST:"Last\: %5.2lf" 

}

#--lower-limit 15 \
#--alt-y-grid \
#--rigid \

graph_tempmix() {
echo "=> Creating png graph - tempmix"
rrdtool graph /var/www/pigoda/tempmix.png \
	-w 785 -h 420 -a PNG \
	--slope-mode \
	--start now-46h --end now \
	--font='DEFAULT:7:' \
	--title="Relation temperature " \
	--watermark="Date `date`" \
	--lower-limit 15 \
	--rigid \
	DEF:vc_temp=/var/db/rrdcache/vc_temp.rrd:vctemp:AVERAGE \
	VDEF:vctemplast=vc_temp,LAST \
	DEF:temp_out=/var/db/rrdcache/tempout.rrd:tempout:AVERAGE \
	VDEF:tempoutlast=temp_out,LAST \
	DEF:temp_in=/var/db/rrdcache/tempin.rrd:tempin:AVERAGE \
	VDEF:tempinlast=temp_in,LAST \
	COMMENT:"Legend\:" \
	LINE2:vc_temp#D40000:"temp vc_core (C)" \
	LINE1:vctemplast#C837AB:"last temp (C)":dashes \
	GPRINT:vc_temp:MAX:"Max vc_core\: %5.2lf\n" \
	AREA:temp_in#FF00FF:"temp inside(C)" \
	GPRINT:temp_in:MAX:"Max inside\: %5.2lf\n" \
	GPRINT:temp_in:MIN:"Min inside\: %5.2lf\n"  \
	GPRINT:temp_in:LAST:"Last inside\: %5.2lf\n"  \
	AREA:temp_out#0080FF:"temp outside(C)\n" \
	LINE1:tempoutlast#FF6600:"temp outside last (C)\n" \
	LINE1:tempinlast#00CCFF:"temp inside last (C)\n" \
	GPRINT:temp_out:MAX:"Max outside\: %5.2lf\n"  \
	GPRINT:temp_out:MIN:"Min outside\: %5.2lf\n"  \
	GPRINT:temp_out:AVERAGE:"Avg outside\: %5.2lf\n" \
	GPRINT:temp_out:LAST:"Last outside\: %5.2lf\n" 

}


graph_vc_temp_daily () {
echo "=> Creating png graph - vc_temp"
rrdtool graph /var/www/pigoda/temperature_daily.png \
	-w 785 -h 120 -a PNG \
	--slope-mode \
	--start 'now-1d' --end now \
	--font='DEFAULT:7:' \
	--title="vc_core temperature whole day" \
	--watermark="Date `date`" \
	--alt-y-grid \
	--rigid \
	DEF:vc_temp=/var/db/rrdcache/vc_temp.rrd:vctemp:AVERAGE \
	VDEF:vctempmax=vc_temp,MAXIMUM \
	AREA:vc_temp#D40000:"temp(C)\l" \
	LINE1:vctempmax#008000:"vc temp max(C)\l" \
	GPRINT:vc_temp:MAX:"Max\: %5.2lf\l" \
	GPRINT:vc_temp:MIN:"Min\: %5.2lf \l"  \
	GPRINT:vc_temp:LAST:"Last\: %5.2lf \l" 
}
graph_vc_temp_pastdays () {
	echo "=> Creating png graph - vc_temp past 3 days"
	rrdtool graph /var/www/pigoda/temperature_3_days.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start 'now-14d' --end now \
		--font='DEFAULT:7:' \
		--title="vc_core temperature past 3 days" \
		--watermark="Date `date`" \
		--alt-y-grid \
		--rigid \
		DEF:vc_temp=/var/db/rrdcache/vc_temp.rrd:vctemp:AVERAGE \
		LINE1:vc_temp#FF00FF:"temp(C)" \
		GPRINT:vc_temp:MAX:"Max\: %5.2lf" 

}

graph_light () {

echo "=> Creating png graph - light"
rrdtool graph /var/www/pigoda/light.png \
	-w 785 -h 120 -a PNG \
	--slope-mode \
	--start 'now-12h' --end now \
	--font='DEFAULT:7:' \
	--title="Light past 12 hours" \
	--watermark="Date `date`" \
	--alt-y-grid \
	--rigid \
	DEF:light=/var/db/rrdcache/light.rrd:light:AVERAGE \
	AREA:light#00FF00:"light \l" \
	GPRINT:light:MAX:"Max\: %5.2lf\l" \
	GPRINT:light:MIN:"Min\: %5.2lf \l" \
	GPRINT:light:AVERAGE:"Average\: %5.2lf\l" 

}

graph_light_weekly () {

echo "=> Creating png graph - light weekly"
rrdtool graph /var/www/pigoda/light_weekly.png \
	-w 785 -h 120 -a PNG \
	--slope-mode \
	--start 'now-14d' --end now \
	--font='DEFAULT:7:' \
	--title="Light past 7 days" \
	--watermark="Date `date`" \
	--alt-y-grid \
	--rigid \
	DEF:light=/var/db/rrdcache/light.rrd:light:AVERAGE \
	AREA:light#00FF00:"light \l" \
	GPRINT:light:MAX:"Max\: %5.2lf\l" \
	GPRINT:light:MIN:"Min\: %5.2lf \l" \
	GPRINT:light:AVERAGE:"Average\: %5.2lf\l" 

}



graph_tempout() {
echo "=> Creating png graph - tempout"
	rrdtool graph /var/www/pigoda/tempout.png \
	-w 785 -h 120 -a PNG \
	--slope-mode \
	--start now-3h --end now \
	--font='DEFAULT:7:' \
	--title="Outside temperature past 3 hours" \
	--watermark="Date `date`" \
	--alt-y-grid \
	--rigid \
	DEF:tempout=/var/db/rrdcache/tempout.rrd:tempout:AVERAGE \
	AREA:tempout#EF500B:"temp(C)\n" \
	GPRINT:tempout:MAX:" Max\: %5.2lf \n" \
	GPRINT:tempout:AVERAGE:" Avg\: %5.2lf \n" \
	GPRINT:tempout:MIN:" Min\: %5.2lf\n" \
	GPRINT:tempout:LAST:"Last\: %5.2lf\n" 
}

graph_tempout_weekly() {
echo "=> Creating png graph - tempout"
rrdtool graph /var/www/pigoda/tempout_weekly.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start now-7d --end now \
--font='DEFAULT:7:' \
--title="Outside temperature past few days" \
--watermark="Date `date`" \
--alt-y-grid \
--rigid \
DEF:tempout=/var/db/rrdcache/tempout.rrd:tempout:AVERAGE \
AREA:tempout#EF500B:"temperature(C)\n" \
GPRINT:tempout:MAX:" Max\: %5.2lf \n" \
GPRINT:tempout:AVERAGE:" Avg\: %5.2lf \n" \
GPRINT:tempout:MIN:" Min\: %5.2lf\n" \
GPRINT:tempout:LAST:"Last\: %5.2lf\n" 
}


graph_pressure() {
echo "=> Creating png graph - pressure"
rrdtool graph /var/www/pigoda/pressure.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start now-1d --end now-10m \
--font='DEFAULT:7:' \
--title="Pressure" \
--watermark="Date `date`" \
--alt-y-grid \
--rigid \
DEF:pressure=/var/db/rrdcache/pressure.rrd:pressure:AVERAGE \
AREA:pressure#008000:"Pressure (hPa)" \
GPRINT:pressure:MAX:"Max\: %5.2lf" \
GPRINT:pressure:MIN:"Min\: %5.2lf" \
GPRINT:pressure:LAST:"Last\: %5.2lf"  \
GPRINT:pressure:AVERAGE:"Avg\: %5.2lf" 

}


graph_pressure_daily() {
echo "=> Creating png graph - pressure daily"
rrdtool graph /var/www/pigoda/pressure_daily.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start now-14d --end now \
--font='DEFAULT:7:' \
--title="Pressure daily" \
--watermark="Date `date`" \
--lower-limit 1008 \
--upper-limit 1023 \
--alt-y-grid \
--rigid \
DEF:pressure=/var/db/rrdcache/pressure.rrd:pressure:AVERAGE \
VDEF:pressuremax=pressure,MAXIMUM \
VDEF:pressuremin=pressure,MINIMUM \
AREA:pressure#008008:"Pressure (hPa)\n" \
LINE1:pressuremax#FF0000:"Pressure max (hPa)\n":dashes \
LINE1:pressuremin#006680:"Pressure min (hPa)\n":dashes \
GPRINT:pressure:MAX:"Max\: %5.2lf\n" \
GPRINT:pressure:MIN:"Min\: %5.2lf\n" \
GPRINT:pressure:LAST:"Last\: %5.2lf\n"  \
GPRINT:pressure:AVERAGE:"Avg\: %5.2lf\n" 

}




update_graphs () {
	gtype=${1:-unknown}
	#if [ "${PLOT_ONLY}" = "yes" ]; then
	#	return
	#fi
	if ! python2 update_rrd.py $gtype; then
		echo "!> failed to update graphs: $gtype"
		exit 3
	fi
}


for cmd in $@ 
do
	case $cmd in
		tempin_now)
			update_graphs tempin;
			graph_tempin;
			;;
		pressure_now)
			update_graphs pressure;
			graph_pressure;
			;;
		all)
			update_graphs pressure;
			update_graphs tempin;
			update_graphs tempout;
			update_graphs light;
			update_graphs vc_temp;
			update_graphs pir;
			graph_tempin;
			graph_tempout;
			graph_pressure;
			graph_light;
			graph_vc_temp;
			graph_vc_temp_daily;
			echo "=> Done!";
			exit 0;
			;;

		exp)
			graph_vc_temp_pastdays
			graph_tempmix
			graph_pressure_daily;
			graph_tempout_weekly
			graph_tempin_weekly;
			graph_pir;
			graph_light_weekly;

			;;
		*)
			echo "$0 commands: [tempin_now|all]"
			exit 64;
			;;
	esac
done


