#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN    48  // Ersetze dies durch den richtigen Pin für dein Board
#define NUMPIXELS  1   // Anzahl der LEDs

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Ersetze diese mit deinem vorhandenen Netzwerk-Namen und Passwort
const char* ssid = "ATHOME";
const char* password = "536c03b0fde40ccfbb7a3f00fecf10b3";

WebServer server(80);

enum Mode {
  OFF,
  SOLID_COLOR,
  ON
};

Mode currentMode = OFF;
int brightness = 128;
uint32_t color = 0xFFFFFF;  // Standardfarbe Weiß

bool firstOn = true;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pixels.begin();

  WiFi.begin(ssid, password);

  Serial.print("Verbinde mit dem WLAN ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WLAN verbunden.");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());

  // Webserver Routen
server.on("/", HTTP_GET, [](){
  String html = "<!DOCTYPE html>\
                 <html>\
                 <head>\
                   <meta charset='UTF-8'>\
                   <title>ESP32 LED Control</title>\
                   <script>\
                     function updateTemperature() {\
                       fetch('/temperature')\
                         .then(response => response.text())\
                         .then(data => {\
                           document.getElementById('temp').innerText = data + ' °C';\
                         });\
                     }\
                     setInterval(updateTemperature, 1000);  // Update alle 5 Sekunden\
                   </script>\
                 </head>\
                 <body onload=\"updateTemperature()\">\
                   <h1>ESP32 LED Control</h1>\
                   <h2 style=\"font-size: 1.5em; font-weight: bold;\">Temperature: <span id='temp'>Loading...</span></h2>\
                   <button onclick=\"setMode('ON')\">On</button>\
                   <button onclick=\"setMode('OFF')\">Off</button>\
                   <br><br>\
                   <label for=\"color\">Choose Color:</label>\
                   <input type=\"color\" id=\"color\" name=\"color\" value=\"#000000\" onchange=\"setColor(this.value)\">\
                   <br><br>\
                   <label for=\"brightness\">Brightness:</label>\
                   <input type=\"range\" id=\"brightness\" name=\"brightness\" min=\"0\" max=\"255\" value=\"" + String(brightness) + "\" onchange=\"setBrightness(this.value)\">\
                   <br><br>\
                   <script>\
                     function setMode(mode) {\
                       fetch('/setMode?mode=' + mode)\
                         .then(response => response.text())\
                         .then(data => console.log(data));\
                     }\
                     function setBrightness(brightness) {\
                       fetch('/setBrightness?value=' + brightness)\
                         .then(response => response.text())\
                         .then(data => console.log(data));\
                     }\
                     function setColor(color) {\
                       fetch('/setColor?value=' + color.substring(1))\
                         .then(response => response.text())\
                         .then(data => console.log(data));\
                     }\
                   </script>\
                 </body>\
                 </html>";
  server.send(200, "text/html", html);
});


  server.on("/setMode", HTTP_GET, [](){
    if (server.hasArg("mode")) {
      String mode = server.arg("mode");
      if (mode == "OFF") currentMode = OFF;
      else if (mode == "ON") {
        currentMode = ON;
        if (firstOn) {
          color = 0xFFFFFF;  // Weiß, wenn das erste Mal eingeschaltet wird
          firstOn = false;
        }
      }
      server.send(200, "text/plain", "Mode set to " + mode);
    } else {
      server.send(400, "text/plain", "Mode not specified");
    }
  });

  server.on("/setBrightness", HTTP_GET, [](){
    if (server.hasArg("value")) {
      brightness = server.arg("value").toInt();
      if (brightness < 0) brightness = 0;
      if (brightness > 255) brightness = 255;
      server.send(200, "text/plain", "Brightness set to " + String(brightness));
    } else {
      server.send(400, "text/plain", "Brightness value not specified");
    }
  });

  server.on("/setColor", HTTP_GET, [](){
    if (server.hasArg("value")) {
      String colorValue = server.arg("value");
      if (colorValue.length() == 6) {
        long number = (long) strtol(&colorValue[0], NULL, 16);
        color = ((number >> 16) & 0xFF) << 16 | ((number >> 8) & 0xFF) << 8 | (number & 0xFF);
        currentMode = SOLID_COLOR;
        firstOn = false; // Farbe wurde explizit gesetzt, daher kein erstes Einschalten mehr
        server.send(200, "text/plain", "Color set to #" + colorValue);
      } else {
        server.send(400, "text/plain", "Invalid color value");
      }
    } else {
      server.send(400, "text/plain", "Color value not specified");
    }
  });

  // Neue Route für Temperatur
  server.on("/temperature", HTTP_GET, [](){
    float temperature = temperatureRead();  // Temperatur auslesen
    server.send(200, "text/plain", String(temperature));
  });

  server.begin();
}

void loop() {
  server.handleClient();

  if (currentMode == ON || currentMode == SOLID_COLOR) {
    pixels.setBrightness(brightness);
    pixels.setPixelColor(0, color);
  } else {
    pixels.setPixelColor(0, pixels.Color(0, 0, 0)); // LED ausschalten
  }
  
  pixels.show();
  delay(50);
}
