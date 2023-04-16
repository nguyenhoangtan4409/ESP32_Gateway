#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <driver/ledc.h>
#include "my_BMP280.h"
const char* ssid     = "ESP_Deviot";
const char* password = "123456789";

const char* serverAddress = "192.168.1.1";
const int serverPort = 80;
uint8_t led = 2;
bool ledstatus = LOW;
int duty = 0;
void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Read BMP280 sensor value without using library");
  //Cấu hình cảm biến BMP280
  init_BMP280(0x76);
  setup_BMP280(normal, SAMPLING_X2, SAMPLING_NONE, FILTER_X16, STANDBY_MS_500);    
  read_Compensation_parameter_storage(); 

  //Cấu hình kênh 0 với tần số 1Hz và độ phân giải 16 bit
  ledcSetup(0, 1, 16);

}

int dutyCycleToH_pulsewidth(int percent){
  int duty = map(percent, 0, 100, 0, 65535);
  return duty;
}
float temp = 20;

void reconnectWiFi() {
  Serial.println("Attempting to reconnect to WiFi...");
  WiFi.disconnect();
  delay(1000);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Reconnected to WiFi.");
}
bool isAutoMode = 0;
void loop() {

  if (WiFi.status() != WL_CONNECTED)
  {
    reconnectWiFi();
  }

  WiFiClient client;
  if (client.connect(serverAddress, serverPort))
  {
    Serial.println("Connected to server");
    while(client.connected())
    {
          if(isAutoMode)
          {
            ledcAttachPin(led, 0);
            ledcWrite(0, dutyCycleToH_pulsewidth(duty));
          }
          else
          {
            ledcDetachPin(led);
            if (ledstatus) digitalWrite(led, HIGH);
            else   digitalWrite(led, LOW);
          }
          //temp = readTemperaturee(); // đọc giá trị gửi đến cho đến khi gặp kí tự xuống dòng \n
          temp += 0.2;
          if(temp > 40) temp = 20;
          // if (client.available()){
              String response = client.readStringUntil('#');
              if (response == "ON") {ledstatus = HIGH; isAutoMode = 0;}
              else if (response == "OFF") {ledstatus = LOW; isAutoMode = 0;}
              else if (response == "0P1") {duty = 0; isAutoMode = 1;}
              else if (response == "25P1") {duty = 25; isAutoMode = 1;}
              else if (response == "75P1") {duty = 75; isAutoMode = 1;}
              else if (response == "temp") client.println(String(temp));

              Serial.printf("Temperature: %0.2f *C\n",temp);
              Serial.printf("Say from Gateway: %s\n", response);
          // }
    }
  }
  else 
  {
    client.stop();
    delay (1000);
  }

}
