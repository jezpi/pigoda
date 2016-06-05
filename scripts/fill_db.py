import rrdtool, time,sys,datetime,sqlite3;
import os
import curses


def printoff(off):
	return time.strftime("%c",time.gmtime(off))

def fill_db_vc_temp(sqlitedb, offset):
	conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
	cur.execute("SELECT * from vc_temp_now WHERE times > ? ORDER BY times", [str(offset)]);
	for row in cur:
		sup=str(row[0])+":"+str(row[1]);
		print sup;
		try:
			rrdtool.update('light.rrd', sup);
		except:
			print "rrd failure!"


def create_db(rrdfile, dsname, offs, step=60):
	try:
		os.remove(rrdfile);
	except:
		win.addstr(6, 0, "Failed to remove "+rrdfile);
		win.refresh();
		return 0;
	rrdtool.create(rrdfile, "--step", str(step), "--start", str(offs),
		"DS:"+dsname+":GAUGE:120:0:24000",
                "RRA:AVERAGE:0.5:1:864000",
                "RRA:AVERAGE:0.5:60:129600",
                "RRA:AVERAGE:0.5:3600:13392")



def fill_db(sqlitedb, query, rrdfile, offset):
	conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
	win.addstr(1, 0, "hi");
	win.refresh();
	failures = 0;
	qcounter = 0;
	tim=0;
	
	cur.execute(query, [str(offset)]);
	for row in cur:
		qcounter=qcounter+1;
		tim=row[0];
		sup=str(row[0])+":"+str(row[1]);
		supp=printoff(tim)+" = "+str(row[1]);
		win.addstr(1, 20,  supp+"\t"+str(qcounter));
		win.refresh();
		try:
			rrdtool.update(rrdfile, sup);
		except:
			failures=failures+1;
			win.addstr(2, 0,  "failure "+str(failures)+" /"+supp);

def get_sqlite_offset(sqlitedb, table, ts='timestamp'):
	conn = sqlite3.connect(sqlitedb);
	cur  = conn.cursor();
	cur.execute("SELECT * FROM "+table+" ORDER BY "+ts+" ASC LIMIT 1;");
	offset=cur.fetchone()[0];
	return int(offset);



if len(sys.argv) > 1 :
	cmd=sys.argv[1];
else:
	print "usage filldb.py [arg]"
	sys.exit(64)

sensorsdb='/var/db/pigoda/sensors.db'
rrdpath="/var/db/rrdcache/"

curses.initscr();
win = curses.newwin(0, 0);
win.insstr(1, 0, "...Press any key to continue...");
win.refresh();
win.getkey();
win.addstr(1, 0, "  Wait                         ");
win.refresh();
#off=1462656009
#off=1463171075
#fill_db_vc_temp('/home/jez/temperature.db', off);
#off=1463179962
#fill_db_pressure('/home/jez/code/MQTT/graph_channel/sensors.db', off);
#fill_db_tempin('/home/jez/code/MQTT/graph_channel/sensors.db', 1462656009 );
#fill_db_pir('/home/jez/code/MQTT/graph_channel/sensors.db', 1464595304 );
if cmd == "pir":
	off=get_sqlite_offset(sensorsdb, 'pir');
	print printoff(off)+"  pir";
elif cmd == "pressure":
	off=get_sqlite_offset(sensorsdb, 'pressure');
	print "Filling pressure starting from: "+printoff(off);
	create_db(rrdpath+"/pressure.rrd", "pressure", off);
	fill_db(sensorsdb, 'SELECT * from pressure WHERE timestamp > ? ORDER BY timestamp', '/var/db/rrdcache/pressure.rrd', off)
	print printoff(off)+"  pressure";
elif cmd == "light":
	off=get_sqlite_offset(sensorsdb, 'light', ts='times');
	wstr="   Start offset:"+printoff(off)+" light";
	win.addstr(1, 0, wstr);
	win.refresh();
	create_db(rrdpath+"/light.rrd", "light", off);
	fill_db(sensorsdb, "SELECT * from light WHERE times > ? ORDER BY times", '/var/db/rrdcache/light.rrd', off);
elif cmd == "tempin":
	off=get_sqlite_offset(sensorsdb, 'temp_in', ts='timestamp');
	wstr="   Start offset:"+printoff(off)+" temperaute inside";
	win.addstr(1, 0, wstr);
	win.refresh();
	create_db(rrdpath+"/tempin.rrd", "tempin", off);
	fill_db(sensorsdb, "SELECT * from temp_in WHERE timestamp > ? ORDER BY timestamp", '/var/db/rrdcache/tempin.rrd', off);
else:
	win.addstr(0, 20, "unknown command "+cmd);



win.insstr(5, 10, "FINISHED!");
win.refresh();
win.getkey();
curses.endwin();
print "done!"
