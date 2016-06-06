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

#### How to use it?
Download the source repository, compile* the sources and install it:

git clone https://github.com/jezpi/pigoda
cd mqtt_rpi
make && make install

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

