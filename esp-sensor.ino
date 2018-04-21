#include <ESP8266WiFi.h>        // https://github.com/esp8266/Arduino
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>       // https://github.com/knolleary/pubsubclient/
#include <DHT.h>                // https://github.com/adafruit/DHT-sensor-library

#include "config.h"

ADC_MODE(ADC_VCC);

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);


void setup() {
  WiFi.mode(WIFI_STA);
  delay(10);
  WiFi.begin(wifi_ssid, wifi_password);
  WiFi.mode(WIFI_STA);
  
  dht.begin();

  float batt = ESP.getVcc() / 1000.0;
  float temp;
  float hum;

  /* Read DHT */
  for(int i = 0; i < 12; i++) {
    temp = dht.readTemperature(false, true);
    hum = dht.readHumidity();
    if (!isnan(temp) && !isnan(hum)) {
      break;
    }
    delay(250);
  }

  /* Wait for WiFi */
  for(int i = 0; i < 12; i++) {
    if(WiFi.status() == WL_CONNECTED) {
      break;
    }
    delay(250);
  }
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  if (!client.connect(sensor_name)) {
    deepsleep();
  }
  client.subscribe(update_topic);

  client.loop();
  
  if (!isnan(temp)) {
    client.publish(temp_topic, String(temp,1).c_str(), true);
  }
  if (!isnan(hum)) {
    client.publish(hum_topic, String(hum,1).c_str(), true);
  }
  if (!isnan(batt)) {
    client.publish(batt_topic, String(batt,3).c_str(), true);
  }
  delay(200);
  client.loop();
  /* Delay to send data... */
  delay(500);
  client.loop();
  delay(500);
  
  deepsleep();
}

void deepsleep() {
  ESP.deepSleep(interval * 60 * 1000000);
  delay(100);
}

void callback(const String topic, byte* payload, unsigned int length) {
  if(topic == update_topic) {
    if ((char)payload[0] == '1') {
      client.publish(update_topic, "0", true);
      delay(1000);
      t_httpUpdate_return ret = ESPhttpUpdate.update(update_path);
      switch(ret) {
        case HTTP_UPDATE_FAILED:
            client.publish(update_topic, "Failed.", true);
            delay(1000);
            break;
        case HTTP_UPDATE_OK:
            break;
      }
    }
  }
}

void loop() {}
