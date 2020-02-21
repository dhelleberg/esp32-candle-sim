#include <Arduino.h>
#include <WiFi.h>
#include <updater.h>
#include <FastLED.h>
#include <MQTT.h>

/*Put your SSID & Password*/
const char* ssid = WSSID;  // Enter SSID here
const char* password = WPWD;  //Enter Password here

const int WIFI_TIMEOUT_RESTART_S=60;
#define PIXEL_PIN 5
#define PIXEL_COUNT 12

//Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN,  NEO_GRBW + NEO_KHZ800);
// Define the array of leds
CRGB leds[PIXEL_COUNT];
#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels

long lastTime;
int interval;
const int EFFECT_INTERVAL = 60 * 1000 * 15;
const int UPDATE_INTERVAL = 60 * 1000 * 5;
const int RECONNECT_INTERVAL = 30 * 1000;



const int MODE_FIRE = 1;
const int MODE_EFFECT = 2;
const int MODE_OFF = 0;
int mode = MODE_EFFECT;

long lastEffect =  0;
long lastUpdateCheck =  0;
long lastReconnect =  0;
int effect = -1;

WiFiClient net;
MQTTClient client;

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  int mqtt_mode = payload.toInt();
  mode = mqtt_mode;
}

void setup() {
   // Setup Serial on 115200 Baud
  Serial.begin(115200);
  Serial.print(ssid);
  FastLED.addLeds<NEOPIXEL, PIXEL_PIN>(leds, PIXEL_COUNT);

  // Start Wifi connection
  WiFi.mode(WIFI_OFF);
  WiFi.setSleep(false);
 
  WiFi.begin(ssid, password);
  Serial.print("[WIFI] Connecting to WiFi ");

  int wifi_connection_time = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    wifi_connection_time++;
    if (wifi_connection_time > WIFI_TIMEOUT_RESTART_S) {
      Serial.println("[WIFI] Run into Wifi Timeout, try restart");
      Serial.flush();
      WiFi.disconnect();
      ESP.restart();
    }
  }
  Serial.println("\n[WIFI] Connected");
  lastReconnect = millis();
  Updater::check_for_update();
  randomSeed(analogRead(0));
  interval = 150;

  Serial.print("\n mqtt.. connecting...");
  client.begin("192.168.178.21",net);
  client.onMessage(messageReceived);
  while (!client.connect("esp-fire", "try", "try")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  client.subscribe("/fire-status");
  client.publish("/esp-fire-version", FIRMWARE_VERSION, true, 2);
}

void SimulateFire ()
{
    long nowmilli = millis();
    if (nowmilli - lastTime >= interval)
    {
      byte LightValue[PIXEL_COUNT * 3];
      lastTime = nowmilli;
      
      for (int i = 0; i < PIXEL_COUNT; i++)
      { // For each pixel...
        LightValue[i * 3] = random(230, 255); // 250
        LightValue[i * 3 + 1] = random(70, 100); // 50
        LightValue[i * 3 + 2] = 0;
      }
      // Switch some lights darker
      byte LightsOff  = random(0, 6);
      for (int i = 0; i < LightsOff; i++)
      {
        byte Selected = random(PIXEL_COUNT);
        LightValue[Selected * 3] = random(40, 60);
        LightValue[Selected * 3 + 1] = random(10, 25);
        LightValue[Selected * 3 + 2] = 0;
      }
      for (int i = 0; i < PIXEL_COUNT; i++)
      { // For each pixel...
        leds[i].setRGB(LightValue[i * 3], LightValue[i * 3 + 1], LightValue[i * 3 + 2]);
        FastLED.show();      
      }
      interval = random(50,200);
    }
  
}
void leds_off() {
    for (int i = 0; i < PIXEL_COUNT; i++)
    { // For each pixel...
      leds[i] = CRGB::Black;
      FastLED.show();   // Send the updated pixel colors to the hardware.
    }
}

bool effect_pink() {
  for (int i = 0; i < PIXEL_COUNT; i++)
  { // For each pixel...
    leds[i].setRGB(255,00,70);
  }
  FastLED.show();   // Send the updated pixel colors to the hardware.
  delay(1000);
  return true;
}
bool effect_pink_flicker() {
  for(int count = 0; count < 10; count++) {
    for (int i = 0; i < PIXEL_COUNT; i++)
    { // For each pixel...
      leds[i].setRGB(255,00,70);
    }
    FastLED.show();   // Send the updated pixel colors to the hardware.
    delay(100);
    leds_off();
    delay(100);
  }
  return true;
}

bool effect_flash() {
  for(int count = 0; count < 20; count++) {
    for (int i = 0; i < PIXEL_COUNT; i++)
    { // For each pixel...
      leds[i].setRGB(255,255,255);
    }
    FastLED.show();   // Send the updated pixel colors to the hardware.
    delay(30);
    leds_off();
    delay(30);
  }
  return true;
}

void renderEffect() {
  bool finished = false;
  //based on effect selection render one special effect
  if( effect = -1)
    effect = random(0,3);
  switch (effect)
  {
    case 0:
      finished = effect_pink();
    break;
    case 1:
      finished = effect_pink_flicker();
    break;
    case 2:
      finished = effect_flash();
    break;
  }
  
  if(finished) {
    //effect done
    finished = false;
    mode = MODE_FIRE;
  }
}
void myFlicker() {
    for(int i = 0; i < PIXEL_COUNT; i++) {
    Serial.println(i);
    leds[i].setRGB(random(180,255), random(20), random(0,10));
    leds[i].nscale8(random(0,200));
    FastLED.show();
    //strip.show();
    delay(random(0,20));
  }
  //delay(2000);
  //strip.clear();
  for(int i = 0; i < PIXEL_COUNT; i++) {
    
    leds[i].nscale8(random(0,200));
    //strip.show();    
  }
  FastLED.show();
}

void loop() {
  client.loop();
  long now = millis();
  if(now > (lastReconnect + RECONNECT_INTERVAL)) {
    lastReconnect = now;
    if(WiFi.status() != WL_CONNECTED)
    {
      Serial.println("WIFI re-connecting...");
      int wifi_retry = 0;
      while(WiFi.status() != WL_CONNECTED && wifi_retry < 5 ) {
        wifi_retry++;
        Serial.println("WiFi not connected. Try to reconnect");
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
        delay(100);
      }
    }
    if (!client.connected()) {
        Serial.println("mqtt client re-connecting...");
        int tries = 0;
        boolean repeat = true;
        while (!client.connect("arduino", "try", "try") && repeat) {
          Serial.print(".");
          delay(100);
          tries++;
          if(tries > 5)
            repeat = false;
        }
        Serial.println("reconnected: "+String(repeat));
        if(repeat)
          client.subscribe("/hello");
    }  
  }
  
  if(now > (lastEffect + EFFECT_INTERVAL))
    mode = MODE_EFFECT;
  switch(mode) {
    case MODE_FIRE:
      SimulateFire();
      break;  
    case MODE_EFFECT:
      lastEffect = now;
      renderEffect();
      break;
    case MODE_OFF:
      //do nothing, sleep for 30 sec
      Serial.println("OFF-Mode-Sleeping...");
      leds_off();
      delay(30*1000);
      Serial.println("continunue...");
      break;
  }
  if(now > (lastUpdateCheck+UPDATE_INTERVAL)) {

    Serial.println("checking for SW-Update");
    Updater::check_for_update();
    lastUpdateCheck = now;
  }
    
}
