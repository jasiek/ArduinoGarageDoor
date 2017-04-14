#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <avr/pgmspace.h>

const byte mac[] = { 0xDE, 0xAD, 0x00, 0x00, 0x00, 0x00 };
const PROGMEM IPAddress server(192, 168, 1, 2);
const char control_topic[] = "control/dead00000000";

EthernetClient client;
PubSubClient mqtt(client);

bool a0, a1, a2;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println(topic);
  Serial.println((char*)payload);
  if (strcmp(topic, control_topic) == 0) {
    if (strstr(payload, "RELAY1") == 0) {
      digitalWrite(8, LOW);
      delay(1000);
      digitalWrite(8, HIGH);
    }
    if (strstr(payload, "RELAY2") == 0) {
      digitalWrite(9, LOW);
      delay(1000);
      digitalWrite(9, HIGH);
    }
  }
}

bool read(int pin) {
  return analogRead(pin) > 750;
}

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac);
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(Ethernet.localIP());
  Serial.println(MQTT_MAX_PACKET_SIZE);

  pinMode(9, OUTPUT);
  pinMode(8, OUTPUT);

  digitalWrite(8, HIGH);
  digitalWrite(9, HIGH);

  mqtt.setServer(server, 1883);
  mqtt.setCallback(callback);

  a0 = read(0);
  a1 = read(1);
  a2 = read(2);
}


void checkForChanges() {
  bool na0, na1, na2;

  na0 = read(0);
  na1 = read(1);
  na2 = read(2);

  StaticJsonBuffer<128> buffer;
  JsonObject &root = buffer.createObject();
  bool report = false;

  if (a0 != na0) {
    root["a0"] = na0;
    report = true;
  }

  if (a1 != na1) {
    root["a1"] = na1;
    report = true;
  }

  if (a2 != na2) {
    root["a2"] = na2;
    report = true;
  }

  if (report) {
    String str;
    root.printTo(str);
    mqtt.publish("device/dead00000000", str.c_str());
  }

  a0 = na0;
  a1 = na1;
  a2 = na2;
}

void loop() {
  Ethernet.maintain();
  while (!mqtt.connected()) {
    Serial.println("connecting");
    if (mqtt.connect("arduino-garage-door-dead00000000")) {
      Serial.println("connected");
      if (mqtt.subscribe(control_topic)) {
        Serial.println(control_topic);
      }
      mqtt.publish("hello", "{}");
    }
    delay(1000);
  }
  mqtt.loop();
  checkForChanges();
  delay(10);
}
