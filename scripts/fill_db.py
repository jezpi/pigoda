import rrdtool, time,os,sys,datetime,sqlite3;
import curses

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



def fill_db_pressure(sqlitedb, offset):
	conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
	cur.execute("SELECT * from pressure WHERE timestamp > ? ORDER BY timestamp", [str(offset)]);
	for row in cur:
		sup=str(row[0])+":"+str(row[1]);
		print sup;
		try:
			rrdtool.update('pressure.rrd', sup);
		except:
			print "rrd failure!"


def fill_db_tempin(sqlitedb, offset):
	conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
	win.insstr(1, 0, "hi");
	win.refresh();
	failures = 0;
	qcounter = 0;
	
	cur.execute("SELECT * from temp_in WHERE timestamp > ? ORDER BY timestamp", [str(offset)]);
	for row in cur:
		qcounter = qcounter +1;
		sup=str(row[0])+":"+str(row[1]);
		win.addstr(1, 0,  sup+"    "+str(qcounter));
		win.refresh();
		try:
			rrdtool.update('tempin.rrd', sup);
		except:
			failures=failures+1;
			win.addstr(2, 0,  "failure "+str(failures));


def fill_db_tempout(sqlitedb, offset):
	conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
	win.addstr(1, 0, "hi");
	win.refresh();
	failures = 0;
	qcounter = 0;
	
	cur.execute("SELECT * from temp_out WHERE timestamp > ? ORDER BY timestamp", [str(offset)]);
	for row in cur:
		qcounter=qcounter+1;
		sup=str(row[0])+":"+str(row[1]);
		win.addstr(1, 6,  sup+"\t"+str(qcounter));
		win.refresh();
		try:
			rrdtool.update('tempout.rrd', sup);
		except:
			failures=failures+1;
			win.addstr(2, 0,  "failure "+str(failures));


def fill_db_pir(sqlitedb, offset):
	conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
	win.addstr(1, 0, "hi");
	win.refresh();
	failures = 0;
	qcounter = 0;
	
	cur.execute("SELECT * from pir WHERE timestamp > ? ORDER BY timestamp", [str(offset)]);
	for row in cur:
		qcounter=qcounter+1;
		sup=str(row[0])+":"+str(row[1]);
		win.addstr(1, 6,  sup+"\t"+str(qcounter));
		win.refresh();
		try:
			rrdtool.update('pir.rrd', sup);
		except:
			failures=failures+1;
			win.addstr(2, 0,  "failure "+str(failures));


def fill_db(sqlitedb, query, rrdfile):
	conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
	win.addstr(1, 0, "hi");
	win.refresh();
	failures = 0;
	qcounter = 0;
	
	cur.execute(query);
	for row in cur:
		qcounter=qcounter+1;
		sup=str(row[0])+":"+str(row[1]);
		win.addstr(1, 6,  sup+"\t"+str(qcounter));
		win.refresh();
		try:
			rrdtool.update(rrdfile, sup);
		except:
			failures=failures+1;
			win.addstr(2, 0,  "failure "+str(failures));

def get_sqlite_offset(sqlitedb, table):
	conn = sqlite3.connect(sqlitedb);
	cur  = conn.cursor();
	cur.execute("SELECT * FROM "+table+" ORDER BY timestamp ASC LIMIT 1;");
	offset=cur.fetchone()[0];
	return int(offset);

off=get_sqlite_offset('/var/db/pigoda/sensors.db', 'pir');
print off;
sys.exit(0);
curses.initscr();
win = curses.newwin(0, 0);
win.insstr(1, 0, "hi");
win.refresh();
win.getkey();
#off=1462656009
#off=1463171075
#fill_db_vc_temp('/home/jez/temperature.db', off);
#off=1463179962
#fill_db_pressure('/home/jez/code/MQTT/graph_channel/sensors.db', off);
#fill_db_tempin('/home/jez/code/MQTT/graph_channel/sensors.db', 1462656009 );
#fill_db_pir('/home/jez/code/MQTT/graph_channel/sensors.db', 1464595304 );
curses.endwin();
print "done!"
