/*  NETPIE ESP8266                                         */
/*  More information visit : https://netpie.io             */

#include "config.h"
#include <ESP8266WiFi.h>
#include <MicroGear.h>
#include <EEPROM.h>

#define SEND_TOPIC1 "/esprelay/state1"
#define SEND_TOPIC2 "/esprelay/state2"
#define SUBSCRIBE_TOPIC1 "/state1"
#define SUBSCRIBE_TOPIC2 "/state2"
#define CHECK_TOPIC1 "/" APPID SUBSCRIBE_TOPIC1
#define CHECK_TOPIC2 "/" APPID SUBSCRIBE_TOPIC2
#define EEPROM_STATE_ADDRESS_1 128
#define EEPROM_STATE_ADDRESS_2 129
#define RELAYPIN_1 13
#define RELAYPIN_2 12
#define BUTTONPIN_1 2
#define BUTTONPIN_2 0
int state1 = 0;
int state2 = 0;
int stateOutdated = 0;
bool reply1 = true;
bool reply2 = true;
unsigned long timer = 0;
unsigned long timeonpress1 = 0;
unsigned long timeonpress2 = 0;
int delayonpress = 700;

WiFiClient client;

MicroGear microgear(client);

void updateIO(int relaypin, int state, int index) {
  Serial.print("updatIO ");
  Serial.print(relaypin);
  Serial.print(" ---------------> ");
  Serial.println(state);
  
  if (state == 1) {
    digitalWrite(relaypin, HIGH);
  }else if (state == 0) {
    digitalWrite(relaypin, LOW);
  }

  if(index==1){
    EEPROM.write(EEPROM_STATE_ADDRESS_1, state);
    EEPROM.commit();
  }else if(index==2){
    EEPROM.write(EEPROM_STATE_ADDRESS_2, state);
    EEPROM.commit();
  }
}

void sendState() {
  if(reply1){ 
    updateIO(RELAYPIN_1, state1, 1);
    Serial.print("Sent state1 : ");
    Serial.println(state1);
    microgear.publish(SEND_TOPIC1,state1);
    reply1 = false;
  }
  if(reply2){ 
    updateIO(RELAYPIN_2, state2, 2);
    Serial.print("Sent state2 : ");
    Serial.println(state2);
    microgear.publish(SEND_TOPIC2,state2);
    reply2 = false;
  }
}

/* If a new message arrives, do this */
void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
    char m = *(char *)msg;
    
    Serial.print("Topic --> ");
    Serial.println((char *)topic);
    Serial.print("Incoming message --> ");
    msg[msglen] = '\0';
    Serial.println((char *)msg);

    if (m == '0' || m == '1') {
      if((String)topic == CHECK_TOPIC1){
        state1 = m=='0'?0:1;
        //updateIO(RELAYPIN_1, state1, 1);
        reply1 = true;
      }else if((String)topic == CHECK_TOPIC2){
        state2 = m=='0'?0:1;
        //updateIO(RELAYPIN_2, state2, 2);
        reply2 = true;
      }
    }else if(m == '?'){
      if((String)topic == CHECK_TOPIC1) reply1 = true;
      else if((String)topic == CHECK_TOPIC2) reply2 = true;
    }else {
      if(m == 'f'){
        if((String)topic == CHECK_TOPIC1) {
          digitalWrite(RELAYPIN_1, HIGH);
          delay(1000);
          state1 = 0;
          reply1 = true;
          //updateIO(RELAYPIN_1, state1, 1);                      
        }
        else if((String)topic == CHECK_TOPIC2) {
          digitalWrite(RELAYPIN_2, HIGH);
          delay(1000);
          state2 = 0;
          reply2 = true;
          //updateIO(RELAYPIN_2, state2, 1);                      
        }
      }
    }
    if (m == '0' || m == '1' || m == '?' || m == 'f') stateOutdated = 1;
}

/* When a microgear is connected, do this */
void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.println("Connected to NETPIE...");
    /* Set the alias of this microgear ALIAS */
    microgear.setAlias(ALIAS);
    microgear.subscribe(SUBSCRIBE_TOPIC1);
    microgear.subscribe(SUBSCRIBE_TOPIC2);
    stateOutdated = 1;
}

void initWiFi(){
    /* Initial WIFI, this is just a basic method to configure WIFI on ESP8266.                       */
    /* You may want to use other method that is more complicated, but provide better user experience */
    if (WiFi.begin(ssid, password)) {
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
    }

    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

static void onButtonPressed1(void) {
    Serial.println("Button Pressed 1");
    if(millis()-timeonpress1 >= delayonpress){
      if (state1 == 1) state1 = 0;
      else state1 = 1;
      
      timeonpress1 = millis();
      stateOutdated = 1;
      reply1 = true;
    }
}

static void onButtonPressed2(void) {
    Serial.println("Button Pressed 2");
    if(millis()-timeonpress2 >= delayonpress){
      if (state2 == 1) state2 = 0;
      else state2 = 1;

      timeonpress2 = millis();
      stateOutdated = 1;
      reply2 = true;
    }
}

void setup() {
    /* Add Event listeners */

    /* Call onMsghandler() when new message arraives */
    microgear.on(MESSAGE,onMsghandler);

    /* Call onConnected() when NETPIE connection is established */
    microgear.on(CONNECTED,onConnected);

    Serial.begin(115200);
    Serial.println("Starting...");
    EEPROM.begin(512);
    timer = millis();
    
    pinMode(RELAYPIN_1, OUTPUT);
    pinMode(RELAYPIN_2, OUTPUT);
    pinMode(BUTTONPIN_1, INPUT);
    pinMode(BUTTONPIN_2, INPUT);
    
    state1 = EEPROM.read(EEPROM_STATE_ADDRESS_1)==1?1:0;
    state2 = EEPROM.read(EEPROM_STATE_ADDRESS_2)==1?1:0;
    updateIO(RELAYPIN_1, state1, 1);
    updateIO(RELAYPIN_2, state2, 2);
    
    attachInterrupt( BUTTONPIN_1, onButtonPressed1, FALLING );
    attachInterrupt( BUTTONPIN_2, onButtonPressed2, FALLING );

    initWiFi();

    /* Initial with KEY, SECRET and also set the ALIAS here */
    microgear.init(KEY,SECRET,ALIAS);

    /* connect to NETPIE to a specific APPID */
    microgear.connect(APPID);
}

void loop() {
    if(WiFi.status()!=WL_CONNECTED){
      WiFi.disconnect();
      initWiFi();
    } else {
        /* To check if the microgear is still connected */
        if (microgear.connected()) {
    
            /* Call this method regularly otherwise the connection may be lost */
            microgear.loop();
    
            if (stateOutdated) sendState();
        }
        else {
            if (millis()-timer >= 5000) {
                Serial.println("connection lost, reconnect...");
                microgear.connect(APPID);
                timer = millis();
            }
        }
    }
}
