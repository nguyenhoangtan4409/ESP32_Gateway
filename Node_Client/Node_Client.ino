#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <driver/ledc.h>
#include "my_BMP280.h"

const char* ssid     = "ESPGateway";
const char* password = "123456789";

// thông tin để kết nối lên gateway
const char* serverAddress = "192.168.1.1";
const int serverPort = 80;

uint8_t led = 2; // cấu hình chân GPIO LED 2
bool ledstatus = LOW;
int duty = 0; // biến lưu trữ giá trị duty cycle để băm xung
float temp = 0; // biến lưu nhiệt độ đọc từ cảm biến

void setup() {
  Serial.begin(115200);

  //kết nối Wifi của Gateway
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
  ledcAttachPin(led, 0);
}

int dutyCycleToH_pulsewidth(int percent){
  int duty = map(percent, 0, 100, 0, 65535);
  return duty;
}

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
bool isBlinkMode = 0;
void loop() {

  if (WiFi.status() != WL_CONNECTED)
  {
    reconnectWiFi();
  }

  WiFiClient client;
  if (client.connect(serverAddress, serverPort)) // nếu kết nối thành công
  {
    Serial.println("Connected to server");
    while(client.connected()) // vòng lặp nếu vẫn còn kết nối với Gateway
    {       
          ledcWrite(0, dutyCycleToH_pulsewidth(duty));
          temp = readTemperaturee(); // đọc nhiệt độ từ cảm biến
          Serial.printf("Temperature: %0.2f *C\n",temp);
        
          if (client.available()){ // nếu dữ có sẵn sàng để đọc chưa
              String response = client.readStringUntil('#'); // đọc yêu cầu từ Gateway
              
              if (response == "ON")         {duty = 100;  isBlinkMode = 0;}
              else if (response == "OFF")   {duty = 0;    isBlinkMode = 0;}
              else if (response == "0P1")   {duty = 0;    isBlinkMode = 1;}
              else if (response == "25P1")  {duty = 25;   isBlinkMode = 1;}
              else if (response == "75P1")  {duty = 75;   isBlinkMode = 1;}
              else if (response == "temp") client.println(String(temp));

              Serial.printf("Say from Gateway: %s\n", response);
          }
          
    }
  }
  else 
  {
    client.stop(); // ngưng kết nối
    delay (1000);
  }

}
