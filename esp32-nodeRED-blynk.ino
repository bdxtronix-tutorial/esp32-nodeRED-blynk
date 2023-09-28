
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <DHT.h>

// Constants
#define DHTPIN 23
#define DHTTYPE DHT11  // DHT 11
#define MOTOR_RELAY_PIN 21

#define BLYNK_TEMPLATE_ID "ID Template Blynk Anda"
#define BLYNK_TEMPLATE_NAME "Nama Template Blynk Anda"
#include <BlynkSimpleEsp32.h>
#define BLYNK_AUTH_TOKEN "Authorization Token Blynk Anda"

const char* ssid = "Nama WiFi Anda";
const char* password = "Password WiFi Anda";
const char* mqtt_server = "IP Address Server Anda";
const int ledPin = 13;

// Global variables
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
int value = 0;
DHT dht(DHTPIN, DHTTYPE);
float temperature = 0;
float humidity = 0;
BlynkTimer timer;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == "esp32/output") {
    Serial.print("Changing output to ");
    if(messageTemp == "on") {
      Serial.println("on");
      digitalWrite(ledPin, HIGH);
    }
    else if(messageTemp == "off") {
      Serial.println("off");
      digitalWrite(ledPin, LOW);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

BLYNK_WRITE(V0) {
  int value = param.asInt();
  if(value == 1) {
    client.publish("esp32/output", "on");
  } else {
    client.publish("esp32/output", "off");
  }
  Blynk.virtualWrite(V1, value);
}

void myTimerEvent() {
  Blynk.virtualWrite(V2, millis() / 1000);
}

void setup() {
  Serial.begin(115200);

  dht.begin();  // initialize DHT
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  pinMode(ledPin, OUTPUT);
  pinMode(MOTOR_RELAY_PIN, OUTPUT);
  digitalWrite(MOTOR_RELAY_PIN, LOW);
  
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  timer.setInterval(1000L, myTimerEvent);
}

void loop() {
  Blynk.run();
  timer.run();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    
    char tempString[8];
    dtostrf(temperature, 1, 2, tempString);
    Serial.print("Temperature: ");
    Serial.println(tempString);
    client.publish("esp32/temperature", tempString);
    Blynk.virtualWrite(V3, temperature);

    char humString[8];
    dtostrf(humidity, 1, 2, humString);
    Serial.print("Humidity: ");
    Serial.println(humString);
    client.publish("esp32/humidity", humString);
    Blynk.virtualWrite(V4, humidity);

    if (humidity < 90) {
      Serial.println("Humidity is below 90%. Activating the motor.");
      digitalWrite(MOTOR_RELAY_PIN, HIGH);
    } else {
      Serial.println("Humidity is 90% or above. Deactivating the motor.");
      digitalWrite(MOTOR_RELAY_PIN, LOW);
    }
  }
}
