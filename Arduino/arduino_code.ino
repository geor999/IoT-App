#include <SPI.h>
#include "DHT.h"
#include <ArduinoMqttClient.h>
#include <Ethernet.h>
#define DHTPIN 2       // which pin we're connected to
#define DHTTYPE DHT22  // DHT 22 (AM2302)
// Update these with values suitable for your network.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
EthernetClient ethClient;
MqttClient mqttClient(ethClient);
//DHT dht;
DHT dht(DHTPIN, DHTTYPE);
const char broker[] = "test.mosquitto.org";
int port = 1883;
const char topic[] = "hum_topic_1a";//topic humidity
const char topic2[] = "temp_topic_2b";//topic temperature
const char topic3[] = "heat_index_topic_3c";//topic heat_index
const char topic4[] = "real_unique_topic_4";
const char controlTopic[] = "alert";  // topic for control messages




//set interval for sending messages (milliseconds)
const long interval = 15000;//15 sec
unsigned long previousMillis = 0;
int count = 0;
//sensorReadPublish data to MQTT broker
void sensorReadPublish() {
  // Read relative humidity (%)
  float h = dht.readHumidity();
  // Read temperature (Celsius)
  float t = dht.readTemperature();
  // Read heat index
  float hic = dht.computeHeatIndex(t, h);
  // Check if data reads are valid numbers
  if (isnan(h) || isnan(t)) {
    Serial.println("Data error from DHT11 sensor!");
    return;
  }
  // publish temperature to topic2
  mqttClient.beginMessage(topic2);
  mqttClient.print(t);
  mqttClient.endMessage();
  // publish humidity to topic
  mqttClient.beginMessage(topic);
  mqttClient.print(h);
  mqttClient.endMessage();
  // publish heatindex to topic3
  mqttClient.beginMessage(topic3);
  mqttClient.print(hic);
  mqttClient.endMessage();
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" oC ");
  Serial.print("\t");
  Serial.print("Heat Index: ");
  Serial.print(hic);
  Serial.print("\n");
}

// interrupt method to handle MQTT messages
void onMessage(int messageSize) {
  String thisPayload, topic;
  topic = mqttClient.messageTopic();
  //create the payload
  while (mqttClient.available()) {
    thisPayload += (char)mqttClient.read();
  }
  //serial prints
  Serial.print("Received Payload: ");
  Serial.println(thisPayload);
  Serial.print("Topic: ");
  Serial.println(topic);
  //initialite pinstate for demonstration
  int pinState = digitalRead(8);
  //check the topic
  if (topic == "alert") {
    //payload is on
    if (thisPayload == "on") {
      pinState = digitalRead(8);
      Serial.print("Pin 8 state before on: ");
      Serial.println(pinState);
      Serial.println("Turn light on");
      //set pin 8 to high
      digitalWrite(8, HIGH);
      pinState = digitalRead(8);
      Serial.print("Pin 8 state after on: ");
      Serial.println(pinState);
    }
    //payload is off
    if (thisPayload == "off" && pinState==1) {
      pinState = digitalRead(8);
      Serial.print("Pin 8 state before off: ");
      Serial.println(pinState);
      Serial.println("Turn light off");
      //set pin 8 to low
      digitalWrite(8, LOW);
      pinState = digitalRead(8);
      Serial.print("Pin 8 state after off: ");
      Serial.println(pinState);
    }
  }
}

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial)
    ;

  // connect to network:
  Ethernet.begin(mac);

  Serial.println("Attempting to connect to the MQTT broker: " + String(broker));

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    while (1)
      ;
  }
  Serial.println("You're connected to the MQTT broker!");

  // Subscribe to the control topic to receive commands
  mqttClient.subscribe(controlTopic);

  // function to handle the interrupts initialization
  mqttClient.onMessage(onMessage);
  // initialize pin 8 for the led
  pinMode(8, OUTPUT);
  
  dht.begin();  //dht start
}

void loop() {
  // call poll() regularly to allow the library to send MQTT keep
  // alive which avoids being disconnected by the broker
  mqttClient.poll();
  //every 15 seconds
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time a message was sent
    previousMillis = currentMillis;
    sensorReadPublish();
    Serial.println();
  }
}