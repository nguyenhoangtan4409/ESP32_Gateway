#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <iostream>
#include <string>

#define A 30   // nhiệt độ
#define B 37 // nhiệt độ

/* điền SSID & Password của ESP32*/
const char* ssid = "ESP_Deviot"; 
const char* password = "123456789";
/* Gắn cho ESP32 IP tĩnh */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

WiFiServer server(80);
WebServer Server_web(8000);

const int maxClients = 1;
WiFiClient clients[maxClients]; // Mảng các kết nối clients
bool clientAvailable[maxClients]; // Mảng đánh dấu kết nối clients có sẵn

bool LED1status = LOW;

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  Serial.println(WiFi.softAPIP());

  delay(100);
  Server_web.on("/", handle_OnConnect);
  Server_web.on("/auto", handle_auto);
  Server_web.on("/manual", handle_manual);
  Server_web.on("/update", handle_update);
  Server_web.onNotFound(handle_NotFound);
  Server_web.begin();
  server.begin();
  Serial.println("HTTP server started");
  for (int i = 0; i < maxClients; i++) {
    clientAvailable[i] = false; // Khởi tạo mảng clientAvailable
  }
}
int up = 0;
String message = "";
float Temperature = 0;

bool isAutoMode = 0;
void loop() {
  Server_web.handleClient();

  for (int i = 0; i < maxClients; i++) {
    if (clients[i] && clients[i].connected()) 
    {
        Serial.print("Client IP address: ");
        Serial.println(clients[i].remoteIP());
        if (up == 1) clients[i].print("temp#");
        else if(isAutoMode){
          if (A < Temperature && Temperature<= B) clients[i].print("25P1#"); // 35 < Temperature <= 37
          else if (Temperature > B) clients[i].print("75P1#");
          else clients[i].print("0P1#");
        }
        else{
          if(LED1status) clients[i].print("ON#");
          else clients[i].print("OFF#");
        }
    } 
   else
    {
     clients[i].stop();
     clientAvailable[i] = false; // Đánh dấu client không sẵn sàng
     Serial.printf("Client %d disconnected\n", i);
    }
  }
  WiFiClient newClient = server.available();
  if (newClient) {
    for (int i = 0; i < maxClients; i++) 
    {
      if (!clientAvailable[i]) // Tìm kiếm vị trí trống để lưu trữ kết nối mới
      { 
        clients[i] = newClient;
        clientAvailable[i] = true; // Đánh dấu client đã sẵn sàng
        Serial.print("New client connected to client ");
        Serial.println(i);
        break;
      }
    }
  }
  delay(500);
}
void readTemp(){
  for (int i = 0; i < maxClients; i++) 
  {
    if (clients[i] && clients[i].connected()) 
    {
       clients[i].print("temp#");
        if (up == 1 /*|| clients[i].available()*/) 
        { // nếu có dữ liệu nhận từ client thì...
          message = clients[i].readStringUntil('\n');
          Server_web.send(200, "text/html", SendHTML(LED1status));
          Temperature = atof(message.c_str());
        }


    }
  }
}
void handle_OnConnect() {
  LED1status = LOW;
  isAutoMode = 0;
  Server_web.send(200, "text/html", SendHTML(LED1status)); 
}
void handle_auto(){
  isAutoMode = 1;
  Server_web.send(200, "text/html", SendHTML(LED1status)); 
}
void handle_manual(){
  isAutoMode = 0;
  LED1status = !LED1status;
  Serial.printf("GPIO2 Status: %d\n", LED1status);
  Server_web.send(200, "text/html", SendHTML(LED1status)); 
}
void handle_update(){
  up++;
  readTemp();
  Server_web.send(200, "text/html", SendHTML(LED1status)); 
  up = 0;
}
void handle_NotFound(){
  Server_web.send(404, "text/plain", "Not found");
}
String SendHTML(uint8_t led1stat) {
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>LED Control</title>\n";
  ptr += "<meta http-equiv=\"refresh\" content=\"30\">\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr += ".button {display: inline-block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px 10px 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-auto {background-color: #f1c40f;width: 120px;}\n";
  ptr += ".button-auto:active {background-color: #f39c12;}\n";
  ptr += ".button-manual {background-color: #f1c40f;width: 120px;}\n";
  ptr += ".button-manual:active {background-color: #f39c12;}\n";
  ptr += ".button-update {background-color: #f39c12; width: 120px;}\n";
  ptr += ".button-update:active {background-color: #e67e22;}\n";
  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<h1>ESP32 Web Server</h1>\n";
  ptr += "<p>";
  if (isAutoMode) {
    ptr += "<span style=\"font-size: 30px;color: #3498db;\">LedMode: Auto</span>";
  } else {
    ptr += "<span style=\"font-size: 30px;color: #f1c40f;\">LedMode: Manual (";
    ptr += String(led1stat ? "Led-On" : "Led-Off") + ")</span>";
  }
  ptr += "</p>\n";
  ptr += "<div>";
  ptr += "<a class=\"button button-auto\" style=\"display: inline-block;\" href=\"/auto\">Auto</a>";
  ptr += "<a class=\"button button-manual\" style=\"display: inline-block; margin-left: 10px;\" href=\"/manual\">Manual</a>\n";
  ptr += "</div>\n";
  ptr += "<p><span style=\"font-size: 30px; font-weight: bold;\">Temperature: " + message + "&#8451;</span></p>\n";
  ptr += "<a class=\"button button-update\" href=\"/update\">Update Temp</a>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

