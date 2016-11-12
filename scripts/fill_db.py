import rrdtool,time,sys,datetime,sqlite3;
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


def create_db(dsname, offs, step=60 ):
        rrdfile=rrdpath+dsname+".rrd"
	try:
		os.remove(rrdfile);
	except:
		win.addstr(6, 0, "Failed to remove "+rrdfile);
		win.refresh();
        finally:
            win.addstr(6, 0, "RRD file: "+rrdfile+" has been removed");
	    win.refresh();

        win.addstr(6, 0, "Created a new RRD file: "+rrdfile+"     ");
        win.refresh();
	rrdtool.create(rrdfile, "--step", str(step), "--start", str(offs),
		"DS:"+dsname+":GAUGE:120:0:24000",
                "RRA:AVERAGE:0.5:1:864000",
                "RRA:AVERAGE:0.5:60:129600",
                "RRA:AVERAGE:0.5:3600:13392")



def fill_rrd_db(sqlitedb, query, rrdfile, offset):
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

def plot_table(tname):
    rrdf=rrdpath+"/"+tname+".rrd";
    offs=get_sqlite_offset(sensorsdb, tname);
    printoff(offs)

    wstr="   Start offset:"+printoff(offs)+" "+tname;
    win.addstr(1, 0, wstr);
    win.refresh();
    create_db(tname, offs, 60);
    sq_query = 'SELECT * from '+tname+' WHERE timestamp > ? ORDER BY timestamp';
    fill_rrd_db(sensorsdb, sq_query, rrdf, offs)



#######
# main
# create and fill rrd database from sqlite


if len(sys.argv) > 1 :
	cmd=sys.argv[1];
else:
	print "usage filldb.py [arg]"
	sys.exit(64)

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

if cmd == "pir":
    plot_table(cmd);
elif cmd == "pressure":
    plot_table(cmd);
elif cmd == "light":
    plot_table(cmd);
elif cmd == "tempin":
    plot_table(cmd);
elif cmd == "tempout":
    plot_table(cmd);
elif cmd == "mic":
    plot_table(cmd);
else:
	win.addstr(0, 20, "unknown command "+cmd);



win.insstr(5, 10, "FINISHED!");
win.refresh();
win.getkey();
curses.endwin();
print "done!"
