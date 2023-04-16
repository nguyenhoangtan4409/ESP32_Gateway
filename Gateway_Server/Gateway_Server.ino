#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

#define A 33   // nhiệt độ
#define B 37 // nhiệt độ
// SSID & Password của ESP32 ở chế độ Access Point để làm Gateway
const char* ssid = "ESPGateway"; 
const char* password = "123456789";
// Gắn cho ESP32 IP tĩnh
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
// Port cho Gateway để trao đổi dữ liệu với Node là 80
WiFiServer server(80);
// Port cho webserver là 8000
WebServer Server_web(8000);

const int maxNode = 1; // Số node tối đa
WiFiClient nodes[maxNode]; // Mảng các kết nối nodes
bool nodeAvailable[maxNode]; // Mảng đánh dấu kết nối nodes có sẵn

bool LED1status = LOW;

bool isBlinkMode = 0;
bool update = 0;

String message = "";
float Temperature = 0;

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  Serial.println(WiFi.softAPIP());

  delay(100);
  //Các phương thức để xử request từ Web - đường dẫn 
  Server_web.on("/", handle_OnConnect);
  Server_web.on("/blink", handle_blink);
  Server_web.on("/manual", handle_manual);
  Server_web.on("/update", handle_update);
  Server_web.onNotFound(handle_NotFound);
  // Khởi động webserver
  Server_web.begin();
  //Khởi động server gateway
  server.begin();

  Serial.println("HTTP server started");
  for (int i = 0; i < maxNode; i++) {
    nodeAvailable[i] = false; // Khởi tạo mảng nodeAvailable
  }
}

void loop() {
  // Xử lý các request
  Server_web.handleClient();

  for (int i = 0; i < maxNode; i++) {
    if (nodes[i] && nodes[i].connected()) 
    {
        // Các IP các node đã kết nối
        Serial.print("Client IP address: ");
        Serial.println(nodes[i].remoteIP());

        if (update == 1) nodes[i].print("temp#");
        else if(isBlinkMode)
        {
          if (A < Temperature && Temperature<= B) nodes[i].print("25P1#"); // 35 < Temperature <= 37
          else if (Temperature > B) nodes[i].print("75P1#");
          else nodes[i].print("0P1#");
        }
        else{
          if(LED1status) nodes[i].print("ON#");
          else nodes[i].print("OFF#");
        }
    } 
   else
    {
     nodes[i].stop();
     nodeAvailable[i] = false; // Đánh dấu client không sẵn sàng
     Serial.printf("Client %d disconnected\n", i);
    }
  }
  WiFiClient newClient = server.available();
  if (newClient) {
    for (int i = 0; i < maxNode; i++) 
    {
      if (!nodeAvailable[i]) // Tìm kiếm vị trí trống để lưu trữ kết nối mới
      { 
        nodes[i] = newClient;
        nodeAvailable[i] = true; // Đánh dấu client đã sẵn sàng
        Serial.print("New client connected to client ");
        Serial.println(i);
        break;
      }
    }
  }
  delay(500);
}
void readTemp(){
  for (int i = 0; i < maxNode; i++) 
  {
    if (nodes[i] && nodes[i].connected()) 
    {
       nodes[i].print("temp#");
        if (update == 1 && nodes[i].available()) // nếu dữ liệu sẵn sàng từ node
        { 
          message = nodes[i].readStringUntil('\n');
          Temperature = atof(message.c_str()); // chuyển đổi từ String --> float
        }
    }
  }
}
/* Các hàm xử lý cho từ request cụ thể*/
void handle_OnConnect() {
  LED1status = LOW;
  isBlinkMode = 0;
  Server_web.send(200, "text/html", SendHTML(LED1status)); 
}
void handle_blink(){
  isBlinkMode = 1;
  Server_web.send(200, "text/html", SendHTML(LED1status)); 
}
void handle_manual(){
  isBlinkMode = 0;
  LED1status = !LED1status;
  Serial.printf("GPIO2 Status: %d\n", LED1status);
  Server_web.send(200, "text/html", SendHTML(LED1status)); 
}
void handle_update(){
  update = 1;
  readTemp();
  Server_web.send(200, "text/html", SendHTML(LED1status)); 
  update = 0;
}
void handle_NotFound(){
  Server_web.send(404, "text/plain", "Not found");
}
String SendHTML(uint8_t led1stat) { // đoạn code HTML đơn giản để hiển thị lên webserver
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>LED Control</title>\n";
  ptr += "<meta http-equiv=\"refresh\" content=\"30\">\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px blink; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px blink 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr += ".button {display: inline-block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px 10px 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-blink {background-color: #f1c40f;width: 120px;}\n";
  ptr += ".button-blink:active {background-color: #f39c12;}\n";
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
  if (isBlinkMode) {
    ptr += "<span style=\"font-size: 30px;color: #3498db;\">LedMode: blink</span>";
  } else {
    ptr += "<span style=\"font-size: 30px;color: #f1c40f;\">LedMode: Manual (";
    ptr += String(led1stat ? "Led-On" : "Led-Off") + ")</span>";
  }
  ptr += "</p>\n";
  ptr += "<div>";
  ptr += "<a class=\"button button-blink\" style=\"display: inline-block;\" href=\"/blink\">blink</a>";
  ptr += "<a class=\"button button-manual\" style=\"display: inline-block; margin-left: 10px;\" href=\"/manual\">Manual</a>\n";
  ptr += "</div>\n";
  ptr += "<p><span style=\"font-size: 30px; font-weight: bold;\">Temperature: " + message + "&#8451;</span></p>\n";
  ptr += "<a class=\"button button-update\" href=\"/update\">Update Temp</a>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

