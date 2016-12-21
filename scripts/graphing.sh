#!/bin/bash 

. ./pigoda_rrd.inc.sh
: ${VERBOSE:=0}

ifverbose () {
	local input
	read input
	[ ${VERBOSE:-0} -eq 1 ] && printf " \`--==> %s\n" "$input"
}

rrd_graph_def () {
	local gname=${1:?too few args}
oIFS=$IFS
IFS='
'
	rrdtool graph $@ \
		--font='DEFAULT:7:Ubuntu' \
		--font='TITLE:11:Ubuntu bold' \
		--font='UNIT:7:Ubuntu' \
		--font='WATERMARK:8:Ubuntu' \
		--font='LEGEND:9:Ubuntu'  \
		--color='BACK#FAFAFA' \
		--lazy \
		--legend-position=south \
		--legend-direction=topdown \
		--grid-dash=1:3 \
		--slope-mode \
		--border=0 | ifverbose
IFS=$oIFS
}

graph_micguernilaptop () {
	name=micguernilaptop
	rrd_graph_def ${PNG_GRAPH_PATH}/${name}.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start 'now-3h' --end 'now-60s' \
		--font='DEFAULT:7:' \
		--title="${name} sensor stats " \
		--watermark="Date `date`" \
		--lower-limit 0 \
		--upper-limit 5000 \
		--alt-y-grid \
		DEF:uval=${RRD_DB_PATH}/${name}.rrd:${name}:AVERAGE \
		VDEF:uvallast=uval,AVERAGE \
		AREA:uval#0000FF:"values\n" \
		LINE1:uvallast#FF00FF:"Last value\n":dashes \
		GPRINT:uval:AVERAGE:"Avg\: %5.2lf\n"  

}



graph_unknown () {
	local name=$1
	echo "=> Creating png graph - ${name}.png"
	rrd_graph_def ${PNG_GRAPH_PATH}/${name}.png \
		-w 785 -h 160 -a PNG \
		--slope-mode \
		--start 'now-3h' --end 'now-300s' \
		--title="${name} sensor stats " \
		--watermark="Date `date`" \
		--alt-y-grid \
		--alt-autoscale \
		--graph-render-mode=normal \
		--grid-dash=1:3 \
		--border=0 \
		DEF:uval=${RRD_DB_PATH}/${name}.rrd:${name}:AVERAGE \
		VDEF:uvalavg=uval,AVERAGE \
		VDEF:uvalmin=uval,MINIMUM \
		VDEF:uvallast=uval,LAST \
		VDEF:uvalmax=uval,MAXIMUM \
		AREA:uval#988FDC:"Values\t" \
		LINE1:uvallast#A50004:"Last value\t":dashes \
		LINE1:uvalmin#FFD63F:"min value\t":dashes \
		LINE1:uvalmax#12B209:"max value\n":dashes \
		GPRINT:uval:AVERAGE:"Avg\:  %5.2lf\n"  \
		GPRINT:uval:LAST:"Last\: %5.2lf\n" \
		GPRINT:uval:MIN:"Min\:  %5.2lf\n" \
		GPRINT:uval:MAX:"Max\:  %5.2lf\n"   

return 1
}

graph_unknown_wkly () {
	name=$1
	echo "=> Creating weekly png graph - \"$name\""
	rrd_graph_def ${PNG_GRAPH_PATH}/${name}_weekly.png \
		-w 785 -h 160 -a PNG \
		--slope-mode \
		--start 'now-7d' --end 'now-300s' \
		--title="${name} sensor stats " \
		--watermark="Date `date`" \
		--alt-y-grid \
		--alt-autoscale \
		--graph-render-mode=normal \
		--grid-dash=1:3 \
		--border=0 \
		DEF:uval=${RRD_DB_PATH}/${name}.rrd:${name}:AVERAGE \
		VDEF:uvalavg=uval,AVERAGE \
		VDEF:uvalmin=uval,MINIMUM \
		VDEF:uvallast=uval,LAST \
		VDEF:uvalmax=uval,MAXIMUM \
		AREA:uval#988FDC:"Values\t" \
		LINE1:uvallast#A50004:"Last value\t":dashes \
		LINE1:uvalmin#FFD63F:"min value\t":dashes \
		LINE1:uvalmax#12B209:"max value\n":dashes \
		GPRINT:uval:AVERAGE:"Avg\:  %5.2lf\n"  \
		GPRINT:uval:LAST:"Last\: %5.2lf\n" \
		GPRINT:uval:MIN:"Min\:  %5.2lf\n" \
		GPRINT:uval:MAX:"Max\:  %5.2lf\n"  

}

graph_unknown_mly () {
	name=$1
	echo "=> Creating monthly png graph - \"${name}_montly.png\""
	rrd_graph_def ${PNG_GRAPH_PATH}/${name}_monthly.png \
		-w 785 -h 160 -a PNG \
		--slope-mode \
		--start 'now-30d' --end 'now-300s' \
		--title="${name} sensor stats " \
		--watermark="Date `date`" \
		--alt-y-grid \
		--alt-autoscale \
		--graph-render-mode=normal \
		--grid-dash=1:3 \
		--border=0 \
		DEF:uval=${RRD_DB_PATH}/${name}.rrd:${name}:AVERAGE \
		VDEF:uvalavg=uval,AVERAGE \
		VDEF:uvalmin=uval,MINIMUM \
		VDEF:uvallast=uval,LAST \
		VDEF:uvalmax=uval,MAXIMUM \
		AREA:uval#988FDC:"Values\t" \
		LINE1:uvallast#A50004:"Last value\t":dashes \
		LINE1:uvalmin#FFD63F:"min value\t":dashes \
		LINE1:uvalmax#12B209:"max value\n":dashes \
		GPRINT:uval:AVERAGE:"Avg\:  %5.2lf\n"  \
		GPRINT:uval:LAST:"Last\: %5.2lf\n" \
		GPRINT:uval:MIN:"Min\:  %5.2lf\n" \
		GPRINT:uval:MAX:"Max\:  %5.2lf\n"  > /dev/null 2>&1

}
graph_unknown_daily () {
	name=$1
	echo "=> Creating daily png graph - \"${name}_daily.png\""
	rrd_graph_def ${PNG_GRAPH_PATH}/${name}_daily.png \
		-w 785 -h 160 -a PNG \
		--slope-mode \
		--start 'now-48h' --end 'now-300s' \
		--title="${name} sensor stats " \
		--watermark="Date `date`" \
		--alt-y-grid \
		--alt-autoscale \
		--graph-render-mode=normal \
		--grid-dash=1:3 \
		--border=0 \
		DEF:uval=${RRD_DB_PATH}/${name}.rrd:${name}:AVERAGE \
		VDEF:uvalavg=uval,AVERAGE \
		VDEF:uvalmin=uval,MINIMUM \
		VDEF:uvallast=uval,LAST \
		VDEF:uvalmax=uval,MAXIMUM \
		AREA:uval#988FDC:"Values\t" \
		LINE1:uvallast#A50004:"Last value\t":dashes \
		LINE1:uvalmin#FFD63F:"min value\t":dashes \
		LINE1:uvalmax#12B209:"max value\n":dashes \
		GPRINT:uval:AVERAGE:"Avg\:  %5.2lf\n"  \
		GPRINT:uval:LAST:"Last\: %5.2lf\n" \
		GPRINT:uval:MIN:"Min\:  %5.2lf\n" \
		GPRINT:uval:MAX:"Max\:  %5.2lf\n"  > /dev/null 2>&1

}

graph_pir() {
echo "=> Creating png graph - pir on badacz"
	rrd_graph_def ${PNG_GRAPH_PATH}/pir.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start 'now-12h' --end 'now-300s' \
		--title="PIR sensor stats - movement rate in one second" \
		--lower-limit 0 \
		--upper-limit 1 \
		--watermark="Date `date`" \
		--alt-y-grid \
		--rigid \
		DEF:movm=${RRD_DB_PATH}/pir.rrd:pir:AVERAGE \
		VDEF:movmavg=movm,AVERAGE \
		VDEF:movmmax=movm,MAXIMUM \
		VDEF:movmmin=movm,MINIMUM \
		VDEF:movmlast=movm,LAST \
		AREA:movm#FFC305:"Movement rate\t" \
		LINE1:movmlast#C70039:"Last movement rate\t":dashes \
		LINE2:movmmax#FF5733:"Max movement rate\t" \
		LINE1:movmmin#581845:"Min movement rate\n":dashes \
		GPRINT:movm:AVERAGE:"Avg\: %5.2lf\n" \
		GPRINT:movm:MAX:"Max\: %5.2lf\n" \
		GPRINT:movm:MIN:"Min\: %5.2lf\n" \
		GPRINT:movm:LAST:"Last\: %5.2lf\n"  

}

graph_humidity () {
	echo "=> Creating png graph - humidity on rpitrois in l24"

	rrd_graph_def ${PNG_GRAPH_PATH}/humidity.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start 'now-3h' --end now \
		--title="Humidity L24" \
		--watermark="Date `date`" \
		--alt-y-grid \
		--lower-limit 0 \
		--upper-limit 100 \
		--rigid \
		DEF:hum=${RRD_DB_PATH}/humidity.rrd:humidity:AVERAGE \
		VDEF:humlast=hum,LAST \
		VDEF:hummax=hum,MAXIMUM \
		AREA:hum#6A4E9D:"Humidity (%)\t" \
		LINE1:hummax#E8006D:"Humidity max(%)\t":dashes \
		LINE1:humlast#81C21D:"Humidity last(%)\n":dashes \
		GPRINT:hum:MAX:"Max\: %5.2lf\n" \
		GPRINT:hum:AVERAGE:"Avg\: %5.2lf\n" \
		GPRINT:hum:MIN:"Min\: %5.2lf\n" \
		GPRINT:hum:LAST:"Last\: %5.2lf\n"  

}

graph_tempin() {
echo "=> Creating png graph - tempin on badacz in made"
	rrd_graph_def ${PNG_GRAPH_PATH}/tempin.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--alt-autoscale \
		--alt-y-grid \
		--no-gridfit \
		--start 'now-3h' --end 'now-300s' \
		--font='DEFAULT:7:' \
		--title="Temperature in the box" \
		--watermark="Date `date`" \
		DEF:temp_in=${RRD_DB_PATH}/tempin.rrd:tempin:AVERAGE \
		VDEF:tempinlast=temp_in,LAST \
		AREA:temp_in#F8A7A7:"temperature(C)\n" \
		LINE1:tempinlast#C00909:"tempemperature last (C)\n":dashes \
		GPRINT:temp_in:MAX:"Max\: %5.2lf\n" \
		GPRINT:temp_in:AVERAGE:"Avg\: %5.2lf\n" \
		GPRINT:temp_in:MIN:"Min\: %5.2lf\n" \
		GPRINT:temp_in:LAST:"Last\: %5.2lf\n"  

}

graph_tempin_guerni() {
echo "=> Creating png graph - temperature in guerni"
	rrd_graph_def ${PNG_GRAPH_PATH}/tempin_guerni.png \
		-w 785 -h 180 -a PNG \
		--slope-mode \
		--start 'now-12h' --end 'now' \
		--upper-limit 25 \
		--lower-limit 15 \
		--title="Temperature l24" \
		--watermark="Date `date`" \
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
echo "=> Creating png graph - temperature inside in guerni"
	rrd_graph_def ${PNG_GRAPH_PATH}/tempin_weekly.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start 'now-7d' --end now \
		--title="Temperature in the box" \
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

graph_vc_temp_badacz() {
	echo "=> Creating png graph - vc_temp"
	rrd_graph_def ${PNG_GRAPH_PATH}/vc_temp_badacz.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start 'now-3h' --end now \
		--title="vc_core temperature on badacz" \
		--watermark="Date `date`" \
		--alt-y-grid \
		--rigid \
		DEF:vc_temp=${RRD_DB_PATH}/vc_temp_badacz.rrd:vc_temp_badacz:AVERAGE \
		LINE1:vc_temp#D40000:"vc_temp(C)" \
		GPRINT:vc_temp:MAX:"Max\: %5.2lf" \
		GPRINT:vc_temp:AVERAGE:"Avg\: %5.2lf" \
		GPRINT:vc_temp:MIN:"Min\: %5.2lf" \
		GPRINT:vc_temp:LAST:"Last\: %5.2lf"  

}

graph_vc_temp_badacz_weekly() {
	echo "=> Creating png graph - vc_temp"
	rrd_graph_def ${PNG_GRAPH_PATH}/vc_temp_badacz_weekly.png \
		-w 785 -h 120 -a PNG \
		--slope-mode \
		--start 'now-7d' --end now \
		--title="vc_core temperature on badacz" \
		--watermark="Date `date`" \
		--alt-y-grid \
		--rigid \
		DEF:vc_temp=${RRD_DB_PATH}/vc_temp_badacz.rrd:vc_temp_badacz:AVERAGE \
		LINE1:vc_temp#D40000:"vc_temp(C)" \
		GPRINT:vc_temp:MAX:"Max\: %5.2lf" \
		GPRINT:vc_temp:AVERAGE:"Avg\: %5.2lf" \
		GPRINT:vc_temp:MIN:"Min\: %5.2lf" \
		GPRINT:vc_temp:LAST:"Last\: %5.2lf"  
}

#--lower-limit 15 \
#--alt-y-grid \
#--rigid \

graph_temprel_custom() {
echo "=> Creating png graph - temperature relation"
rrd_graph_def ${PNG_GRAPH_PATH}/temp_rel.png \
	-w 785 -h 420 -a PNG \
	--slope-mode \
	--start 'now-92h' --end now \
	--title="Relation temperature " \
	--watermark="Date `date`" \
	--alt-autoscale \
	DEF:vc_temp_badacz=${RRD_DB_PATH}/vc_temp_badacz.rrd:vc_temp_badacz:AVERAGE \
	VDEF:vctemplast=vc_temp_badacz,LAST \
	DEF:temp_out=${RRD_DB_PATH}/tempout.rrd:tempout:AVERAGE \
	VDEF:tempoutlast=temp_out,LAST \
	DEF:temp_in=${RRD_DB_PATH}/tempin.rrd:tempin:AVERAGE \
	VDEF:tempinlast=temp_in,LAST \
	DEF:vc_temp_rpitrois=${RRD_DB_PATH}/vc_temp_rpitrois.rrd:vc_temp_rpitrois:AVERAGE \
	DEF:temp_g=${RRD_DB_PATH}/tempin_guerni.rrd:tempin_guerni:AVERAGE \
	COMMENT:"Legend\:\l" \
	LINE2:vc_temp_badacz#D40000:"temp vc_core badacz (C)" \
	LINE2:vc_temp_rpitrois#063F68:"temp vc_core rpitrois (C)" \
	LINE1:vctemplast#C837AB:"last temp (C)":dashes \
	AREA:temp_in#FF00FF:"temp inside(C)\l" \
	AREA:temp_out#0080FF:"temp outside(C)\t" \
	AREA:temp_g#FFB300:"temp l24(C)\t" \
	LINE1:tempoutlast#FF6600:"temp outside last (C)\t" \
	LINE1:tempinlast#00CCFF:"temp inside last (C)\n" \
	GPRINT:vc_temp_badacz:MAX:"Max vc_core\: %5.2lf\n" \
	GPRINT:temp_in:MAX:"Max inside\: %5.2lf\n" \
	GPRINT:temp_out:MAX:"Max outside\: %5.2lf\n"  \
	GPRINT:temp_in:MIN:"Min inside\: %5.2lf\n"  \
	GPRINT:temp_out:MIN:"Min outside\: %5.2lf\n"  \
	GPRINT:temp_out:AVERAGE:"Avg outside\: %5.2lf\n" \
	GPRINT:temp_in:LAST:"Last inside\: %5.2lf\n"  \
	GPRINT:temp_out:LAST:"Last outside\: %5.2lf\n"  

}


graph_vc_temp_badacz_daily () {
echo "=> Creating png graph - vc_temp"
rrd_graph_def ${PNG_GRAPH_PATH}/vc_temp_badacz_daily.png \
	-w 785 -h 120 -a PNG \
	--slope-mode \
	--start 'now-24h' --end now \
	--title="vc_core temperature whole day" \
	--watermark="Date `date`" \
	--alt-y-grid \
	--rigid \
	DEF:vc_temp=${RRD_DB_PATH}/vc_temp_badacz.rrd:vc_temp_badacz:AVERAGE \
	VDEF:vctempmax=vc_temp,MAXIMUM \
	AREA:vc_temp#D40000:"temp(C)\l" \
	LINE1:vctempmax#008000:"vc temp max(C)\l" \
	GPRINT:vc_temp:MAX:"Max\: %5.2lf\l" \
	GPRINT:vc_temp:MIN:"Min\: %5.2lf \l"  \
	GPRINT:vc_temp:LAST:"Last\: %5.2lf \l"  
}

graph_light () {

echo "=> Creating png graph - light in made"
rrd_graph_def ${PNG_GRAPH_PATH}/light.png \
	-w 785 -h 120 -a PNG \
	--slope-mode \
	--start 'now-3h' --end 'now-60s' \
	--title="Light past 3 hours" \
	--watermark="Date `date`" \
	--alt-y-grid \
	--rigid \
	DEF:light=${RRD_DB_PATH}/light.rrd:light:AVERAGE \
	AREA:light#00FF00:"light \l" \
	GPRINT:light:MAX:"Max\: %5.2lf\l" \
	GPRINT:light:MIN:"Min\: %5.2lf \l" \
	GPRINT:light:AVERAGE:"Average\: %5.2lf\l" 

}

graph_light_daily () {
echo "=> Creating png graph - light daily"
rrd_graph_def ${PNG_GRAPH_PATH}/light_daily.png \
	-w 785 -h 120 -a PNG \
	--slope-mode \
	--start 'now-1d' --end now \
	--title="Daily light graphs " \
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
rrd_graph_def ${PNG_GRAPH_PATH}/light_weekly.png \
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
	rrd_graph_def ${PNG_GRAPH_PATH}/tempout.png \
	-w 785 -h 150 -a PNG \
	--start 'now-3h' --end 'now-600s' \
	--title="Temperature in made. Past 3 hours" \
	--watermark="Date `date`" \
	--lower-limit=13	\
	--upper-limit=20	\
	DEF:tempout=${RRD_DB_PATH}/tempout.rrd:tempout:AVERAGE \
	VDEF:tempoutlast=tempout,LAST \
	VDEF:tempoutmin=tempout,MINIMUM \
	VDEF:tempoutmax=tempout,MAXIMUM \
	AREA:tempoutlast#01295F:"temperature last\t"\
	AREA:tempout#FFC300:"temp(C)\t" \
	LINE1:tempoutmin#9B1D20:"temperature min\t":dashes \
	LINE2:tempoutmax#E53100:"temperature max\n" \
	GPRINT:tempout:MAX:" Max\: %5.2lf \n" \
	GPRINT:tempout:AVERAGE:" Avg\: %5.2lf \n" \
	GPRINT:tempout:MIN:" Min\: %5.2lf\n" \
	GPRINT:tempout:LAST:"Last\: %5.2lf\n"  
}

graph_tempout_weekly() {
echo "=> Creating png graph - tempout"
rrd_graph_def ${PNG_GRAPH_PATH}/tempout_weekly.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start now-7d --end now \
--title="Outside temperature past few days" \
--watermark="Date `date`" \
--alt-y-grid \
--rigid \
DEF:tempout=${RRD_DB_PATH}/tempout.rrd:tempout:AVERAGE \
AREA:tempout#EF500B:"temperature(C)\n" \
GPRINT:tempout:MAX:" Max\: %5.2lf \n" \
GPRINT:tempout:AVERAGE:" Avg\: %5.2lf \n" \
GPRINT:tempout:MIN:" Min\: %5.2lf\n" \
GPRINT:tempout:LAST:"Last\: %5.2lf\n"  > /dev/null 2>&1
}


graph_pressure() {
echo "=> Creating png graph - pressure"
rrd_graph_def ${PNG_GRAPH_PATH}/pressure.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start 'now-1d' --end 'now-600s' \
--title="Pressure" \
--watermark="Date `date`" \
--alt-y-grid \
--rigid \
DEF:pressure=${RRD_DB_PATH}/pressure.rrd:pressure:AVERAGE \
VDEF:pressuremax=pressure,MAXIMUM \
VDEF:pressuremin=pressure,MINIMUM \
AREA:pressure#008008:"Pressure (hPa)" \
LINE1:pressuremax#FF0000:"Pressure max (hPa)\n":dashes \
LINE1:pressuremin#006680:"Pressure min (hPa)\n":dashes \
GPRINT:pressure:MAX:"Max\: %5.2lf" \
GPRINT:pressure:MIN:"Min\: %5.2lf" \
GPRINT:pressure:LAST:"Last\: %5.2lf"  \
GPRINT:pressure:AVERAGE:"Avg\: %5.2lf"  


}


graph_pressure_daily() {
	graph_name=${FUNCNAME##graph_}
	dta_name=${graph_name%%_daily}
echo "=> Creating png graph - $dta_name daily"
rrd_graph_def ${PNG_GRAPH_PATH}/${graph_name}.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start 'now-24h' --end now \
--title="Pressure daily" \
--alt-autoscale \
--watermark="Date `date`" \
DEF:pressure=${RRD_DB_PATH}/${dta_name}.rrd:pressure:AVERAGE \
VDEF:pressuremax=pressure,MAXIMUM \
VDEF:pressuremin=pressure,MINIMUM \
VDEF:pressurelast=pressure,LAST \
AREA:pressure#8EA7A7:"Pressure (hPa)\t" \
LINE1:pressuremax#FF0000:"Pressure max (hPa)\t":dashes \
LINE1:pressurelast#C00909:"Pressure last (hPa)\t":dashes \
LINE1:pressuremin#006680:"Pressure min (hPa)\n":dashes \
GPRINT:pressure:MAX:"Max\: %5.2lf\n" \
GPRINT:pressure:MIN:"Min\: %5.2lf\n" \
GPRINT:pressure:LAST:"Last\: %5.2lf\n"  \
GPRINT:pressure:AVERAGE:"Avg\: %5.2lf\n" 

}


graph_pressure_weekly() {
	graph_name=${FUNCNAME##graph_}
	dta_name=${graph_name%%_weekly}
echo "=> Creating png graph - $dta_name weekly"
rrd_graph_def ${PNG_GRAPH_PATH}/${graph_name}.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start 'now-7d' --end now \
--font='DEFAULT:7:' \
--title="Pressure weekly" \
--alt-autoscale \
--watermark="Date `date`" \
DEF:pressure=${RRD_DB_PATH}/${dta_name}.rrd:${dta_name}:AVERAGE \
VDEF:pressuremax=pressure,MAXIMUM \
VDEF:pressuremin=pressure,MINIMUM \
VDEF:pressurelast=pressure,LAST \
AREA:pressure#8EA7A7:"Pressure (hPa)\t" \
LINE1:pressuremax#FF0000:"Pressure max (hPa)\t":dashes \
LINE1:pressurelast#C00909:"Pressure last (hPa)\t":dashes \
LINE1:pressuremin#006680:"Pressure min (hPa)\n":dashes \
GPRINT:pressure:MAX:"Max\: %5.2lf\n" \
GPRINT:pressure:MIN:"Min\: %5.2lf\n" \
GPRINT:pressure:LAST:"Last\: %5.2lf\n"  \
GPRINT:pressure:AVERAGE:"Avg\: %5.2lf\n" 

}




graph_mic_guerni() {
echo "=> Creating png graph - microphone in guerni connected to rpi"
rrd_graph_def ${PNG_GRAPH_PATH}/mic_guerni.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start 'now-1h' --end 'now' \
--title="Mic home" \
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
echo "=> Creating png graph - microphone in made on my laptop "
rrd_graph_def ${PNG_GRAPH_PATH}/micmade.png \
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

html_links() {
	local gtype=${1:?too few args}
	local gname=$2
	local class="${WEB_GRAPH_IMG_CLASS:-img-responsive}"
	local htmlfile=${PNG_GRAPH_PATH}/graphs_${gtype}.html
	local gfile=""

	case $gtype in
		auto)
			gfile="${gname}.png"
			printf '<div class="graph_panel panel panel-default" id="%s_graph_panel">\n' "${gname}"  >> ${htmlfile}
			printf '\t<div class="panel-heading">' >> ${htmlfile}
			printf '%s statistics' "${gname}" >> ${htmlfile}
			printf '</div>\n' >> ${htmlfile}
			printf '\t<div class="panel-body" >\n' >> ${htmlfile}
			printf '\t <a href="%s">\n' ${gfile} >> ${htmlfile}
			printf '\t\t\t<img class="%s" alt="graph_%s_3h" src="%s" />\n' "${class}"  "${gname}" "${gfile}" >> ${htmlfile}
			printf '\t </a>\n' >> ${htmlfile}
			printf '\t</div>\n' >> ${htmlfile}
			printf '</div>\n' >> ${htmlfile}
 			;;
		weekly|daily|monthly)
			gfile="${gname}_${gtype}.png"
			printf '<div class="panel panel-default">\n' >> ${htmlfile}
			printf '\t<div class="panel-heading">\n' >> ${htmlfile}
			printf '\t%s statistics\n' "${gname}" >> ${htmlfile}
			printf '\t</div>\n' >> ${htmlfile}
			printf '\t<div class="panel-body">\n' >> ${htmlfile}
			printf '<a href="%s">\n' ${gfile} >> ${htmlfile}
			printf '<img class="%s" alt="graph_%s_%s" src="%s" />\n' "${class}" "${gname}" "${gtype}" "${gfile}" >> ${htmlfile}
			printf '</a>\n' >> ${htmlfile}
			printf '\t</div>\n' >> ${htmlfile}
			printf '</div>\n' >> ${htmlfile}
			;;
		
	esac

}

usage() {
	echo "graphing.sh commands: [custom|auto|daily|weekly|monthly] [ddname]"
}

######################################
# main
#############

if [ -z ${1}  ]; then
	usage;
	exit 64;
fi

for cmd in $@ 
do
	case $cmd in
		update)
			echo "=> Updating graphs"
			for t in $(sqlite3 /var/db/pigoda/sensorsv2.db '.tables'); 
			do
				echo "==> Updating \"${t}\""
				updatecnt=$(update_graphs $t  | wc -l)
				[ "${updatecnt:-0}" -gt 0 ] && printf '===> %d recs \n' "${updatecnt}"
			done
			;;
		exp)
			echo "obsolete!"
			;;
		monthly)
			echo "=> Creating \"${PNG_GRAPH_PATH}/graphs_monthly.html\""
			:> ${PNG_GRAPH_PATH}/graphs_monthly.html
			for t in $(sqlite3 /var/db/pigoda/sensorsv2.db '.tables'); 
			do 
				echo "==> Drawing \"${t}\""
				html_links "monthly" "${t}"
				eval graph_${t}_monthly > /dev/null 2>&1
				if [ $? -ne 0 ]; then
					printf " \`- - !> Monthly custom graph not available for \"${t}\"\n"
					graph_unknown_mly ${t}
				fi
			done
			;;

		weekly)
			echo "Creating \"${PNG_GRAPH_PATH}/graphs_weekly.html\""
			:> ${PNG_GRAPH_PATH}/graphs_weekly.html
			for t in $(sqlite3 /var/db/pigoda/sensorsv2.db '.tables'); 
			do 
				
				echo "==> Drawing \"${t}\""
				html_links "weekly" "${t}"
				eval graph_${t}_weekly > /dev/null 2>&1
				if [ $? -ne 0 ]; then
					echo "!> Weekly graphs not available for ${t}"
					graph_unknown_wkly ${t}
				fi
			done
			;;
		daily)

			graph_pressure_daily;
			:> ${PNG_GRAPH_PATH}/graphs_daily.html
			for t in $(sqlite3 /var/db/pigoda/sensorsv2.db '.tables'); 
			do 
				
				echo "==> Drawing \"${t}\""
				eval graph_${t}_daily > /dev/null 2>&1
				if [ $? -ne 0 ]; then
					graph_unknown_daily ${t}
				fi
				html_links "daily" "${t}"
			done

			;;
		custom)
			v="temprel"
			eval graph_${v}_custom;
				if [ $? -ne 0 ]; then
					echo "===> Failed to graph temprel"
				fi
			;;
		auto)
			:> ${PNG_GRAPH_PATH}/graphs_auto.html
			for t in $(sqlite3 /var/db/pigoda/sensorsv2.db '.tables'); 
			do 
				
				echo "==> $t"; 
				html_links "auto" "${t}"
				eval graph_$t
				if [ $? -ne 0 ]; then
					echo "!> $t graph failure. Trying to graph by anonymously"
					graph_unknown ${t}
				fi
			done
			;;
			*)
				if [ ${cmd##*_} = "daily" ]; then
					update_graphs ${cmd%%_daily};
				elif [ ${cmd##*_} = "weekly" ]; then
					update_graphs ${cmd%%_weekly};
				else
					update_graphs $cmd;
				fi
				eval graph_$cmd
				if [ $? -ne 0 ]; then
					echo "!> graph failure. Trying to graph by anonymously"
					graph_unknown ${cmd}
				fi
				
			;;
		help)
			usage
			exit 64;
			;;
	esac
done


