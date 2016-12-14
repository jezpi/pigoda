# pigoda - Introduction
Sensing station project on Raspberry Pi. The project is divided into
software and hardware. The software parts consist of 3 main bits:
daemon called *mqtt_rpi* which runs on raspberry Pi and is used to
send statistics. Application called *mqtt_channel* used to collect
sent statistcs. And the plotting software which are basically shell/python
scripts. The hardware part includes board designs as well as designs of 3D 
printed parts (n/a yet). Everything works via *MQTT* protocol. An example
of MQTT client is available in ![here](https://jezpi.github.io/pigoda/sluchacz)

## The software
The standard installation assumes that You have an access to MQTT broker and a
VPS or something like that to collect the statistics. Since I use rrdtool as a plotting
library the statistics require continuity. You should provide a stable connection with accetable
latency (can't be too slow). It is not recommended to run *mqtt_channel* on the same raspberry Pi
where You run *mqtt_rpi* because of SD card overload (It could simply kill Your
SD card). 
### mqtt_rpi
#### Introduction

mqtt_rpi is a broker client as all software pieces written here.
         it sends statistics from previously configured gpios/i2c devices
	 specified in a config file *mqtt_rpi.yaml*. Also it handles some
         basic commands including relay switch on and off.
	   
#### Status:
Ready to use version is available. However it's missing some documentation

Part | Task |  Priority | Status
-----|------|-----------|-------
mqtt_rpi, mqtt_channel|Reconnecting on MQTT error | - | Done 
mqtt_rf | Need to integrate with mqtt_rpi | High | Not started
mqtt_pir | Need to integrate with mqtt_rpi | - |  Done
new pigoda v2 board | redesign, include more gpio protection (with optocoupler and or overcurrent protection)| High | design ongoing
website | Make a simple responsive website | High | In progress
mqtt_channel | Change database engine to MySQL from Sqlite3 | Medium | Not started
mqtt_channel | code checkout | Low | Not started
3D models | Models for boxes used to isolate sensors etc. | Low | Not started

### How it really works?!


**The basic knowledge about MQTT protocol is required**
**More info on https://github.com/mqtt/mqtt.github.io/wiki**

There are 4 applications:

* mqtt_rpi - Gets the data from sensors, process it, and publish on a given _MQTT_ channel.
* mqtt_channel -  Listens on a given channel and stores the obtained data to Sqlite3 database.
* mqtt_rf - Gets statistics from  nRF24+ and pushish it on a given _MQTT_ channel.

#### How to use it?
Download the source repository, compile* the sources and install it:

```
git clone https://github.com/jezpi/pigoda
cd mqtt_rpi
make && make install
```

there are some dependencies like libyaml libmosquitto libsqlite3 and libbsd
The application is located in /usr/bin/mqtt_rpi. It installs the configuration
file placed in /etc/mqtt_rpi.yaml. The syntax is a YAML syntax. You need a MQTT
server which defaults to 172.17.17.4, change it to Your server. You have to configure
daemon mode in /etc/mqtt_rpi.yaml ( *mqtt_rpi* does not detach from terminal by default and it prints loads
of debug messages) like this:

<code>
daemon: true
</code>

### mqtt_channel
Ready to use/develop version is available and there's no documentation 

Working version available

### mqtt_pir

Ready to use/develop version is available

### mqtt_rf
Under development (Proof of concept version already works and stores data into db, however
the code needs to be polished a bit)




## The hardware
### pinheader board
In a directory eagle of github repository there are available boards designs ready to fabricate.
The width of connections is thick on purpose. More information will be available soon.

## Notes
It takes 58seconds to start rpi
350mA while it's running and ~400mA on a startup
