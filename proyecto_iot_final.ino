#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h> 
#include <DHT.h>
#include <DHT_U.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include "variables_amazon.h"
extern const char PRIVATE_KEY[] PROGMEM;
extern const char CERTIFICATE[] PROGMEM;
extern const char AMAZON_ROOT_CA1[] PROGMEM;
//VARIABLES PARA LA INCUBADORA
LiquidCrystal_I2C lcd(0x27,16,2); // se inicializa la pantalla LCD

int SENSOR_TEMPERATURA = 18;
int TEMPERATURA;
int HUMEDAD;
int FOCO=25;
int VENTILADOR=26;

DHT dht(SENSOR_TEMPERATURA, DHT11);

//VARIABLES PARA MQTT, ALEXA
//const char* WIFI_SSID = "AXS_2.4G_NqGUkP";
//const char* WIFI_PASS = "u4EE4mpX";
const char* WIFI_SSID = "UCB-PREMIUM";
const char* WIFI_PASS = "lacatoucb";

const char* MQTT_BROKER = "a3rd2nwokwd1rw-ats.iot.us-east-2.amazonaws.com";
const int MQTT_BROKER_PORT = 8883;

const char* MQTT_CLIENT_ID = "cesar.mendez@ucb.edu.bo";

const char* GET_ACCEPTED_TOPIC = "$aws/things/mqtt_basico/shadow/get/accepted";
const char* UPDATE_ACCEPTED_TOPIC = "$aws/things/mqtt_basico/shadow/update/accepted";

const char* GET_TOPIC = "$aws/things/mqtt_basico/shadow/get";
const char* UPDATE_TOPIC = "$aws/things/mqtt_basico/shadow/update";


WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

String incubatorState = "unknown";
String fanState = "unknown";
String lightbulbState = "unknown";

void setIncubatorState(String str){
  incubatorState = str;
  if (incubatorState == "off") {
    digitalWrite(FOCO,HIGH);
    digitalWrite(VENTILADOR,HIGH);
    fanState = "off";
    lightbulbState = "off";
  } else if (incubatorState == "on") {
    takeDataIncubator();
  }
  reportIncubatorState();
}

StaticJsonDocument<JSON_OBJECT_SIZE(10)> outputDoc;
char outputBuffer[200];
//obtenemos el shadow
void getShadow() {
  mqttClient.publish(GET_TOPIC, String("").c_str());
}

//reportamos al MQTT el estado del led
void reportIncubatorState() {

  outputDoc["state"]["reported"]["id"] = "001";
  outputDoc["state"]["reported"]["lightbulbState"] = lightbulbState;
  outputDoc["state"]["reported"]["fanState"] = fanState;
  outputDoc["state"]["reported"]["temperature"] = TEMPERATURA;
  outputDoc["state"]["reported"]["humidity"] = HUMEDAD;
  outputDoc["state"]["reported"]["eggsQuantity"] = 8;
  outputDoc["state"]["reported"]["incubatorState"] = incubatorState;

  Serial.println("EL SHADOW REPORTED ES: ");
  serializeJson(outputDoc, outputBuffer);
  Serial.println(outputBuffer);

  mqttClient.publish(UPDATE_TOPIC, outputBuffer);

}

StaticJsonDocument<JSON_OBJECT_SIZE(64)> inputDoc;

void callback(const char* topic, byte* payload, unsigned int lenght) {
  String message;
  for (int i = 0; i < lenght; i++) {
    //obtenemos el shadow completo
    message += String((char) payload[i]);
  }
  Serial.println("Message from topic " + String(topic) + ":" + message);
  if (String(topic) == GET_ACCEPTED_TOPIC) {
    DeserializationError err = deserializeJson(inputDoc, payload);
    if (!err) {
      String str = String(inputDoc["state"]["delta"]["incubatorState"].as<const char*>());
      if (!str.isEmpty()) setIncubatorState(str);
    }
  } else if (String(topic) == UPDATE_ACCEPTED_TOPIC) {
    DeserializationError err = deserializeJson(inputDoc, payload);
    if (!err) {
      String str = String(inputDoc["state"]["desired"]["incubatorState"].as<const char*>());
      if (!str.isEmpty()) setIncubatorState(str);
    }
  } 
}
//conexion con el cliente MQTT
boolean mqttClientConnect() {
  Serial.println("Connecting to " + String(MQTT_BROKER));
  if (mqttClient.connect(MQTT_CLIENT_ID)) {
    Serial.println("Connected to " + String(MQTT_BROKER));

    if (mqttClient.subscribe(GET_ACCEPTED_TOPIC)) {
      Serial.println("Subscribed to " + String(GET_ACCEPTED_TOPIC));
    }
    else Serial.println("Can't subscribe to " + String(GET_ACCEPTED_TOPIC));

    if (mqttClient.subscribe(UPDATE_ACCEPTED_TOPIC)) {
      Serial.println("Subscribed to " + String(UPDATE_ACCEPTED_TOPIC));
    }
    else Serial.println("Can't subscribe to " + String(UPDATE_ACCEPTED_TOPIC));

    delay(2000);
    Serial.println("Getting shadow");
    getShadow();
  } else {
    Serial.println("Cant't connecto to " + String(MQTT_BROKER));
  }
  return mqttClient.connected();
}

void takeDataIncubator() {
  if (incubatorState == "on") {
    lcd.backlight();
    TEMPERATURA = dht.readTemperature();
    HUMEDAD = dht.readHumidity();
    delay(500);
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Temperatura:  ");
    lcd.print(TEMPERATURA);
    lcd.setCursor(0, 1);
    lcd.print("Humedad:      ");
    lcd.print(HUMEDAD);
    lcd.print("%");

    if (TEMPERATURA >= 20) {
        digitalWrite(FOCO, HIGH); //off
        digitalWrite(VENTILADOR, LOW); //on
        fanState = "on";
        lightbulbState = "off";
        delay(100);
    } else {
        digitalWrite(FOCO, LOW); //on
        digitalWrite(VENTILADOR, HIGH); //off
        fanState = "off";
        lightbulbState = "on";
        delay(100);
    }
  } else {
    digitalWrite(FOCO, HIGH); //off
    digitalWrite(VENTILADOR, HIGH); //off
    lcd.clear(); 
    lcd.noBacklight();
  }
}


void setup() {
  Serial.begin(115200);
  
  dht.begin();
  lcd.init();
  lcd.backlight();
  //definimos el foco y el ventilador como salidas
  pinMode(FOCO,OUTPUT);
  pinMode(VENTILADOR,OUTPUT);

  Serial.println("Connecting to " + String(WIFI_SSID));
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Can't connect to " + String(WIFI_SSID));
    while (1)  delay(200);
  }
  Serial.println("Connected to " + String(WIFI_SSID));

  wifiClient.setCACert(AMAZON_ROOT_CA1);
  wifiClient.setCertificate(CERTIFICATE);
  wifiClient.setPrivateKey(PRIVATE_KEY);

  mqttClient.setBufferSize(4096);
  mqttClient.setServer(MQTT_BROKER, MQTT_BROKER_PORT);
  mqttClient.setCallback(callback);
}

unsigned long previousConnectMillis = 0;

void loop() {
  //ALEXA y MQTT
  unsigned long now = millis();
  if (!mqttClient.connected()) {
    if (now - previousConnectMillis >= 2000) {
      previousConnectMillis = now;
      if (mqttClientConnect()) previousConnectMillis = 0;
      else delay(1000);
    }
  } else { // Connected to the MQTT Broker
    mqttClient.loop();
    takeDataIncubator();
    delay(20);
  }
}
