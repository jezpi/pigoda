#include <SPI.h>

#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>


String inputString;
boolean inputComplete = false;
boolean radioDebug = false;
boolean dataDebug = false;
const int DHT_pin = 4;
const int CE_pin = 7;
const int CSN_pin = 8;

#define DHTTYPE           DHT11     // DHT 11 
RF24 radio(CE_pin, CSN_pin);
DHT_Unified dht(DHT_pin, DHTTYPE);
byte addresses[][6] = {"1Node","2Node"};

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
uint32_t delayMS;
struct msg {
  uint32_t type;
#define M_TYPE_HUM 0x4
#define M_TYPE_TEMP 0x2
#define M_TYPE_GAS 0x8
  float val;
  uint32_t msize;
};
void setup() {sensor_t sensor;
  
  // put your setup code here, to run once:
  Serial.begin(115200);  
  Serial.flush();
  radio.begin();
  dht.begin();
  printf_begin();
  inputString.reserve(255);
  radio.setRetries(15,15);
  radio.setPALevel(RF24_PA_MIN);
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1,addresses[0]);
  radio.startListening();
  radio.printDetails();
  Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" *C");  
  Serial.println("------------------------------------");
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Humidity");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");  
  Serial.println("------------------------------------");
  // Set delay between sensor readings based on sensor details.
  Serial.print("DHT11 sensor delay: ");
  Serial.println(sensor.min_delay);
  Serial.print("ACTIVE> ");
  delayMS = sensor.min_delay / 1000;
}

void loop() {
  // put your main code here, to run repeatedly:
          unsigned long rawMessage; sensors_event_t event;  
          
  dht.temperature().getEvent(&event);
  
  float temp=event.temperature;
//  Serial.print("Temperature: ");
dht.humidity().getEvent(&event);
  float hum=event.relative_humidity;
  struct msg m;
  float mq2;
  mq2= analogRead(A0);
  if (dataDebug) {
    Serial.print("temperature=");
    Serial.println(temp);
    Serial.print("humidity=");
    Serial.println(hum);
    Serial.print("gas=");
    Serial.println(mq2);
  }

      if ( radio.available() )
      {
        // Fetch the payload, and see if this was the last one.
        radio.read( &rawMessage, sizeof(unsigned long) );

        // Spew it
        if (radioDebug) {
         printf("Got a request %d...\n",rawMessage); 
        }
        
        if (rawMessage == M_TYPE_HUM) {
          m.type = M_TYPE_HUM;
          m.val = hum;
          m.msize = sizeof(struct msg);
          if (radioDebug) {
            printMsg(&m);
          }
          
        } else if (rawMessage == M_TYPE_TEMP) {
            m.type = M_TYPE_TEMP;
            m.val =  temp;
            m.msize = sizeof(struct msg);
            if (radioDebug) {
              printMsg(&m);
            }

        } else if (rawMessage == M_TYPE_GAS) {
          m.type = M_TYPE_GAS;
          m.val = mq2;
          m.msize = sizeof(struct msg);
            if (radioDebug) {
              printMsg(&m);
            }
        }
        radio.stopListening();
        radio.write(&m, sizeof(m));
        radio.startListening();
      }
  if (inputComplete) {
    if (inputString == "radioDebug") {
      if (radioDebug == true) {
        radioDebug = false;
      } else {
        radioDebug = true;
      }
      
    } else if (inputString == "dataDebug") {
      if (dataDebug) {
        dataDebug = false;
      } else {
        dataDebug = true;
      }
    } else if (inputString == "help") {
      Serial.println("commands [radioDebug|help]");      
    } else {
      Serial.print("invalid command ");
      Serial.println(inputString);
      Serial.println("valid commands [radioDebug|dataDebug|help]");      

    }
    inputComplete = false;
    inputString = "";
  }
  delay(delayMS);
 
}

int printMsg(struct msg *m)
{
  String dataname;
  int siz = sizeof(*m);
  if (m->type == M_TYPE_TEMP) {
    dataname = "temperature";
    
  } else if (m->type == M_TYPE_HUM) {
    dataname = "humidity";
  } else if (m->type == M_TYPE_GAS) {
    dataname = "gas";
  }
 //printf("Sending %s %f\n", dataname, m->val);
 Serial.print("sending "); Serial.println(dataname);
  printf("sending a message m.type=%d / %d bytes\n", m->type, siz);
  Serial.println(sizeof(struct msg));
              
}


void serialEvent() {
  while(Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      inputComplete = true;  
    } else {
      inputString += inChar;
    }
    }
}

