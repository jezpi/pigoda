#!/usr/bin/env /usr/bin/python2

import rrdtool,time,sys,datetime,sqlite3;
import os
import curses


def printoff(off):
	return time.strftime("%c",time.gmtime(off))



def create_rrd_db(dsname, offs, step=60 ):
        rrdf=rrdpath+"/"+dsname+".rrd"
	try:
		os.remove(rrdf);
	except:
		win.addstr(6, 0, "Failed to remove "+rrdf);
		win.refresh();
        finally:
            win.addstr(6, 0, "RRD file: "+rrdf+" has been removed");
	    win.refresh();

        #win.addstr(7, 0, "Created a new RRD file: "+rrdfile+"     ");
        #win.refresh();
	try:
		rrdtool.create(str(rrdf), "--step", str(step), "--start", str(offs),
		"DS:"+str(dsname)+":GAUGE:120:0:24000",
                "RRA:AVERAGE:0.5:1:864000",
                "RRA:AVERAGE:0.5:60:129600",
                "RRA:AVERAGE:0.5:3600:13392")
	except Exception as e:
        	win.addstr(7, 0, "Failed to create a new RRD file: "+rrdf+"     "+str(e));
		win.insstr(9, 0, "                               ");
        	win.refresh();
		curses.endwin();
		exit();
		




def fill_rrd_db(conn, query, rrdfile, offset):
	#conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
	win.addstr(1, 0, "  ");
	win.refresh();
	failures = 0;
	qcounter = 0;
	tim=0;
	
	cur.execute(query);
	for row in cur:
		qcounter=qcounter+1;
		tim=row[0];
		sup=str(row[0])+":"+str(row[1]);
		supp=printoff(tim)+" = "+str(row[1]);
		win.addstr(1, 20,  supp+"\t"+str(qcounter));
		win.refresh();
		try:
			rrdtool.update(str(rrdfile), sup);
		except Exception as e:
			failures=failures+1;
			win.addstr(2, 0,  "failure "+str(failures)+" /"+supp);
			win.addstr(10, 0, "ERR: "+str(e));

def get_sqlite_offset(conn, table, ts='timestamp'):
	#conn = sqlite3.connect(sqlitedb);
	cur  = conn.cursor();
	sqquery = "SELECT timestamp FROM "+str(table)+" ORDER BY timestamp ASC LIMIT 1;";
	cur.execute(sqquery);
	try:
		offset=cur.fetchone()[0];
	except:
		print "Failed to get offset"+sqquery;
		exit();
		offset=-1;
	return int(offset);

def plot_table(conn, tname):
    rrdf=rrdpath+"/"+tname+".rrd";
    offs=get_sqlite_offset(conn, tname);
    printoff(offs)
    if offs < 0:
	return 0;

    wstr="   Start offset:"+printoff(offs)+" "+tname;
    win.addstr(1, 0, wstr);
    win.refresh();
    create_rrd_db(tname, offs, 60);
    sq_query = 'SELECT * from '+tname+' ORDER BY timestamp ASC';
    fill_rrd_db(conn, sq_query, rrdf, offs)



#######
# main
# create and fill rrd database from sqlite


if len(sys.argv) > 1 :
	cmd=sys.argv[1];
else:
	cmd= ""

sensorsdb='/var/db/pigoda/sensorsv2.db'
rrdpath="/var/db/pigoda/rrd/"


curses.initscr();
win = curses.newwin(0, 0);
win.insstr(1, 0, "...Press any key to continue...");
#win.insstr(2, 0, rrdpath);
win.refresh();
win.getkey();
win.addstr(1, 0, "  Wait                         ");
win.refresh();

try:
	conn = sqlite3.connect(sensorsdb);
except:
	print "Failed to connect with DB"

cur = conn.cursor();
cur.execute("select tbl_name from sqlite_master;");
for row in cur:
	#sup=str(row[0])+":"+str(row[1]);
	win.addstr(8, 0,  "Plotting "+row[0]);
	plot_table(conn, row[0]);





win.insstr(15, 10, "FINISHED!");
win.refresh();
win.getkey();
curses.endwin();
print "done!"
