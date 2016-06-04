#!/usr/bin/env /usr/bin/python2

import rrdtool, time,os,sys,datetime,sqlite3;


def update_db_vc_temp(sqlitedb):
	conn = sqlite3.connect(sqlitedb);
	st = os.stat('temperature.rrd')
	print str(st.st_mtime);
	cur = conn.cursor();
	cur.execute("SELECT * from vc_temp_now WHERE qtime > ?", [str(st.st_mtime)]);
	for row in cur:
		sup=str(row[0])+":"+str(row[1]);
		#print sup;
		rrdtool.update('temperature.rrd', sup);
	conn.close()



def update_db_tempin(sqlitedb):
	st = os.stat('tempin.rrd');
	conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
	cur.execute("SELECT * from temp_in WHERE timestamp > ?", [str(st.st_mtime)]);
	for row in cur:
		sup=str(row[0])+":"+str(row[1]);
		#print sup;
		rrdtool.update('tempin.rrd', sup);
	conn.close()

def update_db_tempout(sqlitedb):
	st = os.stat('tempout.rrd');
	conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
	cur.execute("SELECT * from temp_out WHERE timestamp > ?", [str(st.st_mtime)]);
	for row in cur:
		sup=str(row[0])+":"+str(row[1]);
		#print sup;
		rrdtool.update('tempout.rrd', sup);
	conn.close()

def update_db_light(sqlitedb):
	st = os.stat('light.rrd');
	conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
	cur.execute("SELECT * from light WHERE times > ?", [str(st.st_mtime)]);
	for row in cur:
		sup=str(row[0])+":"+str(row[1]);
		#print sup;
		rrdtool.update('light.rrd', sup);
	conn.close()

def update_db_pressure(sqlitedb):
	st = os.stat('pressure.rrd');
	conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
	cur.execute("SELECT * from pressure WHERE timestamp > ?", [str(st.st_mtime)]);
	for row in cur:
		sup=str(row[0])+":"+str(row[1]);
		#print sup;
		rrdtool.update('pressure.rrd', sup);
	conn.close()

def update_db_pir(sqlitedb):
	st = os.stat('pir.rrd');
	conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
	cur.execute("SELECT * from pir WHERE timestamp > ?", [str(st.st_mtime)]);
	for row in cur:
		sup=str(row[0])+":"+str(row[1]);
		#print sup;
		rrdtool.update('pir.rrd', sup);
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


dbfile="/home/jez/code/MQTT/graph_channel/sensors.db";
if sys.argv[1] == "vc_temp":
	print "=> Updating vc_temp";
	try:
		update_db_vc_temp('/home/jez/temperature.db')
	except:
		print "Failed to update vc_temp"
elif sys.argv[1] == "tempin":
	print "=> Updating tempin";
	try:
		update_db(dbfile, "temp_in", "tempin.rrd");
		#update_db_tempin(dbfile)
	except:
		print "Failed to update tempin"
elif sys.argv[1] == "tempout":
	print "=> Updating tempout";
	try:
		update_db(dbfile, "temp_out", "tempout.rrd");
		#update_db_tempout(dbfile)
	except:
		print "failed to update tempout"
elif sys.argv[1] == "light":
	print "=> Updating light";
	try:
		update_db(dbfile, "light", "light.rrd", "times");
		#update_db_light(dbfile)
	except:
		print "failed to update light"
elif sys.argv[1] == "pressure":
	print "=> Updating pressure";
	try:
		update_db(dbfile, "pressure", "pressure.rrd");
		#update_db_pressure(dbfile)
	except:
		print "failed to update pressure"
elif sys.argv[1] == "pir":
	print "=> Updating pir";
	try:
		update_db(dbfile, "pir", "pir.rrd");
		#update_db_pir(dbfile)
	except:
		print "failed to update pir"
else:
	print "unknown arg"

