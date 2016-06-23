# pigoda - Introduction
Sensing station project on Raspberry Pi. The project is divided into
software (i.e. mqtt_rpi, mqtt_channel, and rrdtool handling scripts) and
hardware - boards design as well as design of 3D printed pieces (n/a yet).

The version available on github still have lots of defaults from my
personal lab. *IT IS NOT RECOMMENDED TO RUN IT WITHOUT LOOKING INTO
THE SOURCE CODE AND CONFIGURATION FILE*


## The software
The standard installation assumes that You have two machines, one is a 
raspberry pi which sensors connected to specified pins and second is
a mosquitto server which broadcasts information on a configured channel.
### mqtt_rpi
#### Introduction

mqtt_rpi is 
#### Status:
Ready to use version is available. However it's missing some documentation

Part | Task |  Priority
-----|------|--------
mqtt_rpi, mqtt_channel|Reconnecting on MQTT error | High 
mqtt_pir, mqtt_rf | Both need to integrate with mqtt_rpi | High
new pigoda v2 board | redesign, include more gpio protection (with optocoupler and or overcurrent protection)| design ongoing
website | has to be done | High
mqtt_channel | Change database engine to MySQL from Sqlite3 | Medium
mqtt_channel | code checkout | Low
3D models | Models for boxes used to isolate sensors etc. | Low 

### How it really works?!

![graph](https://jezpi.github.io/pigoda/pigoda_howto.svg)

**The basic knowledge about MQTT protocol is required**
**More info on https://github.com/mqtt/mqtt.github.io/wiki**

There are 4 applications:

* mqtt_rpi - Gets the data from sensors, process it, and publish on _MQTT_ channel.
* mqtt_channel -  Listens on a given channel and stores the obtained data to Sqlite3 database.
* mqtt_pir - Gets statistics from pir sensor and publish it on a given _MQTT_ channel.
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


