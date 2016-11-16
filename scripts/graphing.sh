#!/bin/sh

. ./pigoda_rrd.inc.sh
dt=`date +%s`
offs=$((dt-10800))

graph_micguernilaptop () {
	name=micguernilaptop
	echo "=> Creating png graph - $name"
	rrdtool graph ${RRD_GRAPH_PATH}/${name}.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start 'now-3h' --end 'now-60s' \
		--font='DEFAULT:7:' \
		--title="${name} sensor stats " \
		--watermark="Date `date`" \
		--lower-limit 0 \
		--upper-limit 5000 \
		--alt-y-grid \
		--rigid \
		DEF:uval=${RRD_DB_PATH}/${name}.rrd:${name}:AVERAGE \
		VDEF:uvallast=uval,AVERAGE \
		AREA:uval#0000FF:"values\n" \
		LINE1:uvallast#FF00FF:"Last value\n":dashes \
		GPRINT:uval:AVERAGE:"Avg\: %5.2lf\n" 

}

graph_unknown () {
	name=$1
	echo "=> Creating png graph - $name"
	rrdtool graph ${RRD_GRAPH_PATH}/${name}.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start 'now-3h' --end 'now-60s' \
		--font='DEFAULT:7:' \
		--title="${name} sensor stats " \
		--watermark="Date `date`" \
		--alt-y-grid \
		--rigid \
		DEF:uval=${RRD_DB_PATH}/${name}.rrd:${name}:AVERAGE \
		VDEF:uvallast=uval,AVERAGE \
		AREA:uval#0000FF:"values\n" \
		LINE1:uvallast#FF00FF:"Last value\n":dashes \
		GPRINT:uval:AVERAGE:"Avg\: %5.2lf\n" 

}

graph_pir() {
echo "=> Creating png graph - pir"
	rrdtool graph ${RRD_GRAPH_PATH}/pir.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start 'now-12h' --end now \
		--font='DEFAULT:7:' \
		--title="PIR sensor stats - movement rate in one second" \
		--watermark="Date `date`" \
		--alt-y-grid \
		--rigid \
		DEF:movm=${RRD_DB_PATH}/pir.rrd:pir:AVERAGE \
		VDEF:movmlast=movm,AVERAGE \
		AREA:movm#0000FF:"Movement rate\n" \
		LINE1:movmlast#FF00FF:"Average movement rate\n":dashes \
		GPRINT:movm:AVERAGE:"Avg\: %5.2lf\n" 

}

graph_tempin() {
echo "=> Creating png graph - tempin"
	rrdtool graph ${RRD_GRAPH_PATH}/tempin.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start 'now-3h' --end now \
		--font='DEFAULT:7:' \
		--title="Temperature inside" \
		--watermark="Date `date`" \
		--alt-y-grid \
		--rigid \
		DEF:temp_in=${RRD_DB_PATH}/tempin.rrd:tempin:AVERAGE \
		VDEF:tempinlast=temp_in,LAST \
		AREA:temp_in#0000FF:"temperature(C)\n" \
		LINE1:tempinlast#FF00FF:"tempemperature last (C)\n":dashes \
		GPRINT:temp_in:MAX:"Max\: %5.2lf\n" \
		GPRINT:temp_in:AVERAGE:"Avg\: %5.2lf\n" \
		GPRINT:temp_in:MIN:"Min\: %5.2lf\n" \
		GPRINT:temp_in:LAST:"Last\: %5.2lf\n" 
}

graph_tempin_guerni() {
echo "=> Creating png graph - guerni"
	rrdtool graph ${RRD_GRAPH_PATH}/tempin_guerni.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start 'now-3h' --end now \
		--font='DEFAULT:7:' \
		--title="Temperature inside - guerni" \
		--watermark="Date `date`" \
		--alt-y-grid \
		--rigid \
		DEF:temp_in=${RRD_DB_PATH}/tempin_guerni.rrd:tempin_guerni:AVERAGE \
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
	rrdtool graph ${RRD_GRAPH_PATH}/tempin_weekly.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start 'now-3h' --end now \
		--font='DEFAULT:7:' \
		--title="Temperature inside - past 14 days" \
		--watermark="Date `date`" \
		--alt-y-grid \
		--rigid \
		DEF:temp_in=${RRD_DB_PATH}/tempin.rrd:tempin:AVERAGE \
		VDEF:tempinlast=temp_in,LAST \
		AREA:temp_in#0000FF:"temp(C)\n" \
		LINE1:tempinlast#FF00FF:"temp(C)\n":dashes \
		GPRINT:temp_in:MAX:"Max\: %5.2lf\n" \
		GPRINT:temp_in:AVERAGE:"Avg\: %5.2lf\n" \
		GPRINT:temp_in:MIN:"Min\: %5.2lf\n" 
}

graph_vc_temp() {
	echo "=> Creating png graph - vc_temp"
	rrdtool graph ${RRD_GRAPH_PATH}/temperature.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start 'now-3h' --end now \
		--font='DEFAULT:7:' \
		--title="vc_core temperature " \
		--watermark="Date `date`" \
		--alt-y-grid \
		--rigid \
		DEF:vc_temp=${RRD_DB_PATH}/vc_temp.rrd:vctemp:AVERAGE \
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
rrdtool graph ${RRD_GRAPH_PATH}/tempmix.png \
	-w 785 -h 420 -a PNG \
	--slope-mode \
	--start 'now-3h' --end now \
	--font='DEFAULT:7:' \
	--title="Relation temperature " \
	--watermark="Date `date`" \
	--lower-limit 15 \
	--rigid \
	DEF:vc_temp=${RRD_DB_PATH}/vc_temp.rrd:vctemp:AVERAGE \
	VDEF:vctemplast=vc_temp,LAST \
	DEF:temp_out=${RRD_DB_PATH}/tempout.rrd:tempout:AVERAGE \
	VDEF:tempoutlast=temp_out,LAST \
	DEF:temp_in=${RRD_DB_PATH}/tempin.rrd:tempin:AVERAGE \
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
rrdtool graph ${RRD_GRAPH_PATH}/temperature_daily.png \
	-w 785 -h 120 -a PNG \
	--slope-mode \
	--start 'now-3h' --end now \
	--font='DEFAULT:7:' \
	--title="vc_core temperature whole day" \
	--watermark="Date `date`" \
	--alt-y-grid \
	--rigid \
	DEF:vc_temp=${RRD_DB_PATH}/vc_temp.rrd:vctemp:AVERAGE \
	VDEF:vctempmax=vc_temp,MAXIMUM \
	AREA:vc_temp#D40000:"temp(C)\l" \
	LINE1:vctempmax#008000:"vc temp max(C)\l" \
	GPRINT:vc_temp:MAX:"Max\: %5.2lf\l" \
	GPRINT:vc_temp:MIN:"Min\: %5.2lf \l"  \
	GPRINT:vc_temp:LAST:"Last\: %5.2lf \l" 
}
graph_vc_temp_pastdays () {
	echo "=> Creating png graph - vc_temp past 3 days"
	rrdtool graph ${RRD_GRAPH_PATH}/temperature_3_days.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start 'now-14d' --end 'now-60s' \
		--font='DEFAULT:7:' \
		--title="vc_core temperature past 3 days" \
		--watermark="Date `date`" \
		--alt-y-grid \
		--rigid \
		DEF:vc_temp=${RRD_DB_PATH}/vc_temp.rrd:vctemp:AVERAGE \
		LINE1:vc_temp#FF00FF:"temp(C)" \
		GPRINT:vc_temp:MAX:"Max\: %5.2lf" 

}

graph_light () {

echo "=> Creating png graph - light"
rrdtool graph ${RRD_GRAPH_PATH}/light.png \
	-w 785 -h 120 -a PNG \
	--slope-mode \
	--start 'now-3h' --end 'now-60s' \
	--font='DEFAULT:7:' \
	--title="Light past 12 hours" \
	--watermark="Date `date`" \
	--alt-y-grid \
	--rigid \
	DEF:light=${RRD_DB_PATH}/light.rrd:light:AVERAGE \
	AREA:light#00FF00:"light \l" \
	GPRINT:light:MAX:"Max\: %5.2lf\l" \
	GPRINT:light:MIN:"Min\: %5.2lf \l" \
	GPRINT:light:AVERAGE:"Average\: %5.2lf\l" 

}

graph_light_weekly () {

echo "=> Creating png graph - light weekly"
rrdtool graph ${RRD_GRAPH_PATH}/light_weekly.png \
	-w 785 -h 120 -a PNG \
	--slope-mode \
	--start 'now-7d' --end now \
	--font='DEFAULT:7:' \
	--title="Light past 7 days" \
	--watermark="Date `date`" \
	--alt-y-grid \
	--rigid \
	DEF:light=${RRD_DB_PATH}/light.rrd:light:AVERAGE \
	AREA:light#00FF00:"light \l" \
	GPRINT:light:MAX:"Max\: %5.2lf\l" \
	GPRINT:light:MIN:"Min\: %5.2lf \l" \
	GPRINT:light:AVERAGE:"Average\: %5.2lf\l" 

}



graph_tempout() {
echo "=> Creating png graph - tempout"
	rrdtool graph ${RRD_GRAPH_PATH}/tempout.png \
	-w 785 -h 120 -a PNG \
	--slope-mode \
	--start 'now-3h' --end now \
	--font='DEFAULT:7:' \
	--title="Outside temperature past 3 hours" \
	--watermark="Date `date`" \
	--alt-y-grid \
	--rigid \
	DEF:tempout=${RRD_DB_PATH}/tempout.rrd:tempout:AVERAGE \
	AREA:tempout#EF500B:"temp(C)\n" \
	GPRINT:tempout:MAX:" Max\: %5.2lf \n" \
	GPRINT:tempout:AVERAGE:" Avg\: %5.2lf \n" \
	GPRINT:tempout:MIN:" Min\: %5.2lf\n" \
	GPRINT:tempout:LAST:"Last\: %5.2lf\n" 
}

graph_tempout_weekly() {
echo "=> Creating png graph - tempout"
rrdtool graph ${RRD_GRAPH_PATH}/tempout_weekly.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start now-7d --end now \
--font='DEFAULT:7:' \
--title="Outside temperature past few days" \
--watermark="Date `date`" \
--alt-y-grid \
--rigid \
DEF:tempout=${RRD_DB_PATH}/tempout.rrd:tempout:AVERAGE \
AREA:tempout#EF500B:"temperature(C)\n" \
GPRINT:tempout:MAX:" Max\: %5.2lf \n" \
GPRINT:tempout:AVERAGE:" Avg\: %5.2lf \n" \
GPRINT:tempout:MIN:" Min\: %5.2lf\n" \
GPRINT:tempout:LAST:"Last\: %5.2lf\n" 
}


graph_pressure() {
echo "=> Creating png graph - pressure"
rrdtool graph ${RRD_GRAPH_PATH}/pressure.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start 'now-1d' --end 'now-600s' \
--font='DEFAULT:7:' \
--title="Pressure" \
--watermark="Date `date`" \
--alt-y-grid \
--rigid \
DEF:pressure=${RRD_DB_PATH}/pressure.rrd:pressure:AVERAGE \
AREA:pressure#008000:"Pressure (hPa)" \
GPRINT:pressure:MAX:"Max\: %5.2lf" \
GPRINT:pressure:MIN:"Min\: %5.2lf" \
GPRINT:pressure:LAST:"Last\: %5.2lf"  \
GPRINT:pressure:AVERAGE:"Avg\: %5.2lf" 

}


graph_pressure_daily() {
echo "=> Creating png graph - pressure daily"
rrdtool graph ${RRD_GRAPH_PATH}/pressure_daily.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start 'now-24h' --end now \
--font='DEFAULT:7:' \
--title="Pressure daily" \
--watermark="Date `date`" \
--lower-limit 1008 \
--upper-limit 1023 \
--alt-y-grid \
--rigid \
DEF:pressure=${RRD_DB_PATH}/pressure.rrd:pressure:AVERAGE \
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

poligraph() {

#--lower-limit 0 \
#--upper-limit 5000 \
echo "=> Creating png graph - $1 "
rrdtool graph ${RRD_GRAPH_PATH}/$1.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start 'now-1h' --end now \
--font='DEFAULT:7:' \
--title="$1" \
--watermark="Date `date`" \
--alt-y-grid \
--rigid \
DEF:$1=${RRD_DB_PATH}/$1.rrd:$1:AVERAGE \
VDEF:valmax=$1,MAXIMUM \
VDEF:valmin=$1,MINIMUM \
AREA:$1#008008:"mic\n" \
LINE1:valmax#FF0000:"$1 max \n":dashes \
LINE1:valmin#006680:"$1 min \n":dashes \
GPRINT:$1:MAX:"Max\: %5.2lf\n" \
GPRINT:$1:MIN:"Min\: %5.2lf\n" \
GPRINT:$1:LAST:"Last\: %5.2lf\n"  \
GPRINT:$1:AVERAGE:"Avg\: %5.2lf\n" 

}

graph_mic_guerni() {
echo "=> Creating png graph - mic guerni"
rrdtool graph ${RRD_GRAPH_PATH}/mic_guerni.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start now-1h --end now \
--font='DEFAULT:7:' \
--title="Mic guerni" \
--watermark="Date `date`" \
--lower-limit 0 \
--upper-limit 5000 \
--alt-y-grid \
--rigid \
DEF:mic=${RRD_DB_PATH}/mic_guerni.rrd:mic_guerni:AVERAGE \
VDEF:micmax=mic,MAXIMUM \
VDEF:micmin=mic,MINIMUM \
AREA:mic#008008:"mic\n" \
LINE1:micmax#FF0000:"mic max \n":dashes \
LINE1:micmin#006680:"mic min \n":dashes \
GPRINT:mic:MAX:"Max\: %5.2lf\n" \
GPRINT:mic:MIN:"Min\: %5.2lf\n" \
GPRINT:mic:LAST:"Last\: %5.2lf\n"  \
GPRINT:mic:AVERAGE:"Avg\: %5.2lf\n" 

}

graph_micmade() {
echo "=> Creating png graph - micmade "
rrdtool graph ${RRD_GRAPH_PATH}/micmade.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start 'now-2h' --end 'now-60s' \
--font='DEFAULT:7:' \
--title="Mic made" \
--watermark="Date `date`" \
--lower-limit 0 \
--upper-limit 5000 \
--alt-y-grid \
--rigid \
DEF:mic=${RRD_DB_PATH}/micmade.rrd:micmade:AVERAGE \
VDEF:micmax=mic,MAXIMUM \
VDEF:micmin=mic,MINIMUM \
AREA:mic#008008:"mic\n" \
LINE1:micmax#FF0000:"mic max \n":dashes \
LINE1:micmin#006680:"mic min \n":dashes \
GPRINT:mic:MAX:"Max\: %5.2lf\n" \
GPRINT:mic:MIN:"Min\: %5.2lf\n" \
GPRINT:mic:LAST:"Last\: %5.2lf\n"  \
GPRINT:mic:AVERAGE:"Avg\: %5.2lf\n" 

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

#update_graphs $1
#eval graph_$1
graph_all () {
for rrdf in $(ls /var/db/pigoda/rrd/*.rrd);
do
	f=`basename ${rrdf%%.rrd}`
	update_graphs $f
	poligraph $f
done
exit
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
		pir)
			graph_pir;
			;;
		mic)
			graph_mic;
			;;
		light)
			graph_light;
			;;
		pressure)
			graph_pressure;
			;;

		all)
			update_graphs light;
			graph_light;
			update_graphs pressure;
			graph_pressure;
			update_graphs tempin;
			graph_tempin;
			update_graphs tempout;
			graph_tempout;
			update_graphs pir;
			graph_pir;
			update_graphs mic_guerni;
			graph_mic_guerni;
			update_graphs micmade;
			graph_micmade;
			update_graphs tempin_guerni;
			graph_tempin_guerni;
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
		batch)
			graph_all;
			;;
		everything)
			:> /var/www/default/mq_graphs/graphs.html
			for t in $(sqlite3 /var/db/pigoda/sensorsv2.db '.tables'); 
			do 
				
				echo $t; 
				printf '<img src="./%s.png" />\n' "${t}" >> /var/www/default/mq_graphs/graphs.html
				update_graphs $t;
				eval graph_$t
				if [ $? -ne 0 ]; then
					echo "!> graph failure. Trying to graph by anonymously"
					graph_unknown ${t}
				fi
			done
			;;
		*)
			echo "$0 commands: [tempin_now|all]"
			exit 64;
			;;
	esac
done


