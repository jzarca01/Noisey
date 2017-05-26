#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Adafruit_NeoPixel.h>

#include <ESP8266HTTPClient.h>

//for LED status
#include <Ticker.h>
#include "Timer.h"

#define urlApi "http://0360276d.ngrok.io/"
#define NUMPIXELS      24
#define SEUIL          1

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, 12, NEO_GRB + NEO_KHZ800);

const int delayval = 80;
unsigned int wheelpos = 0;
unsigned int strength=0;
unsigned int sample = 0;
unsigned int sampleavg = 1024;
bool stat = 0;

Ticker ticker;
Timer t;

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

void sendPostRequest(char *url, char *message) {
  HTTPClient http;
  Serial.println("sendPostRequest");
  Serial.println(message);
  Serial.println(url);
  http.begin(urlApi);
  http.addHeader("Content-Type", "application/json");
  http.POST("{\"message\":\"Coucou\"}");
  http.writeToStream(&Serial);
  http.end();
}


void animate (){
  for(int i=0;i<NUMPIXELS;i++){
    if(strength<=SEUIL) pixels.setPixelColor(i, pixels.Color(0,0,0));
    else{
      if(i==wheelpos){
        pixels.setPixelColor(i,                pixels.Color(constrain(1*strength-SEUIL,1,255),0,0));
        pixels.setPixelColor((i+1)%NUMPIXELS,  pixels.Color(constrain(1.15*strength-SEUIL,1,255),0,0));
        pixels.setPixelColor((i+2)%NUMPIXELS,  pixels.Color(constrain(1.3*strength-SEUIL,1,255),0,0));
        pixels.setPixelColor((i+3)%NUMPIXELS,  pixels.Color(constrain(1.45*strength-SEUIL,2,255),0,0));
        pixels.setPixelColor((i+4)%NUMPIXELS,  pixels.Color(constrain(1.6*strength-SEUIL,2,255),0,0));
        pixels.setPixelColor((i+5)%NUMPIXELS,  pixels.Color(constrain(1.75*strength-SEUIL,2,255),0,0));
        pixels.setPixelColor((i+6)%NUMPIXELS,  pixels.Color(constrain(2*strength-SEUIL,3,255),0,0));
        pixels.setPixelColor((i+7)%NUMPIXELS,  pixels.Color(constrain(2.25*strength-SEUIL,3,255),0,0));
        pixels.setPixelColor((i+8)%NUMPIXELS,  pixels.Color(constrain(2.5*strength-SEUIL,3,255),0,0));
        pixels.setPixelColor((i+9)%NUMPIXELS,  pixels.Color(constrain(2.75*strength-SEUIL,5,255),0,0));
        pixels.setPixelColor((i+10)%NUMPIXELS, pixels.Color(constrain(2.85*strength-SEUIL,5,255),0,0));
        pixels.setPixelColor((i+11)%NUMPIXELS, pixels.Color(constrain(3*strength-SEUIL,10,255),0,0));
      }
      else if(i>wheelpos+11 || i<wheelpos) pixels.setPixelColor(i, pixels.Color(0,0,0));
    }
  }
  pixels.show();
  wheelpos=(wheelpos+1)%(NUMPIXELS-1);
}

int measure(){
   unsigned long startMillis= millis();
   unsigned int signalMax = 0;
   unsigned int signalMin = 1024;
 
   while(millis() - startMillis < 50){// Sample window width in mS (50 mS = 20Hz)
      int sample = analogRead(A0);
      if (sample < 1024){ 
         if (sample > signalMax)      signalMax = sample;  // save just the max levels
         else if (sample < signalMin) signalMin = sample;  // save just the min levels
      }
   }
   if(signalMax-signalMin<sampleavg)   sampleavg = constrain(signalMax-signalMin,0,1023); // 10 = bruit intrinsèque

   return sample;
}

void updatesat(){
     if(stat) digitalWrite(LED_BUILTIN, HIGH);
     else digitalWrite(LED_BUILTIN, LOW);
     strength=sampleavg-6;
     Serial.print(strength);
     Serial.print("\t");
     strength=map(strength,0,1023,0, 255);
     Serial.print(strength);  
     Serial.print("\n"); 
     sampleavg=1024;
     stat=!stat;
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
    
  //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  pixels.begin();
  Serial.begin(9600);
  t.every(delayval, animate);
  t.every(5000, updatesat);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();
  
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("Noisey")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
  char messageToApi[] = "{\"message\":\"Coucou\"}";
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);
  sendPostRequest(urlApi, messageToApi);
}

void loop() {
  // put your main code here, to run repeatedly:
  int measureResult = 0;
  char test[] = "";
   measureResult = measure();
   //test += measureResult;
   t.update();
   //sendPostRequest(urlApi, test);

}

