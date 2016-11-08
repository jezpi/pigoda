#!/usr/bin/env /usr/bin/python2

import rrdtool, time,os,sys,datetime,sqlite3;


def update_db_vc_temp(sqlitedb):
	conn = sqlite3.connect(sqlitedb);
	st = os.stat('/var/db/rrdcache/vc_temp.rrd')
	print str(st.st_mtime);
	cur = conn.cursor();
	cur.execute("SELECT * from vc_temp_now WHERE qtime > ?", [str(st.st_mtime)]);
	for row in cur:
		sup=str(row[0])+":"+str(row[1]);
		#print sup;
		rrdtool.update('/var/db/rrdcache/vc_temp.rrd', sup);
	conn.close()




def update_db(sqlitedb, table, rrdfile, ts='timestamp'):
	st = os.stat(rrdfile);
	conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
	cur.execute("SELECT * from "+table+" WHERE "+ts+" > ?", [str(st.st_mtime)]);
	for row in cur:
		sup=str(row[0])+":"+str(row[1]);
		#print sup;
		rrdtool.update(rrdfile, sup);
	conn.close()

#######
# main
#

dbfile="/var/db/pigoda/sensors.db";
rrddbpath="/var/db/pigoda/rrd/";
if sys.argv[1] == "vc_temp":
	print "=> Updating vc_temp";
	try:
		update_db_vc_temp('/var/db/pigoda/vc_temp.db')
	except:
		print "Failed to update vc_temp"
elif sys.argv[1] == "tempin":
	print "=> Updating tempin";
	update_db(dbfile, "temp_in", rrddbpath+"/tempin.rrd");
	try:
		update_db(dbfile, "temp_in", rrddbpath+"/tempin.rrd");
		#update_db_tempin(dbfile)
	except:
		print "Failed to update tempin"
elif sys.argv[1] == "tempout":
	print "=> Updating tempout";
	try:
		update_db(dbfile, "temp_out", rrddbpath+"/tempout.rrd");
		#update_db_tempout(dbfile)
	except:
		print "failed to update tempout"
elif sys.argv[1] == "light":
	print "=> Updating light";
	try:
		update_db(dbfile, "light", rrddbpath+"/light.rrd", "times");
		#update_db_light(dbfile)
	except:
		print "failed to update light"
elif sys.argv[1] == "pressure":
	print "=> Updating pressure";
	try:
		update_db(dbfile, "pressure", rrddbpath+"/pressure.rrd");
		#update_db_pressure(dbfile)
	except:
		print "failed to update pressure"
elif sys.argv[1] == "pir":
	print "=> Updating pir";
	try:
		update_db(dbfile, "pir", rrddbpath+"/pir.rrd");
		#update_db_pir(dbfile)
	except:
		print "failed to update pir"
else:
	print "unknown arg"

