/*
 Example using the SparkFun HX711 breakout board with a scale
 By: Nathan Seidle
 SparkFun Electronics
 Date: November 19th, 2014
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

 This example demonstrates basic scale output. See the calibration sketch to get the calibration_factor for your
 specific load cell setup.

 This example code uses bogde's excellent library: https://github.com/bogde/HX711
 bogde's library is released under a GNU GENERAL PUBLIC LICENSE

 The HX711 does one thing well: read load cells. The breakout board is compatible with any wheat-stone bridge
 based load cell which should allow a user to measure everything from a few grams to tens of tons.
 Arduino pin 2 -> HX711 CLK
 3 -> DAT
 5V -> VCC
 GND -> GND

 The HX711 board can be powered from 2.7V to 5V so the Arduino 5V power should be fine.

*/

#include "HX711.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <Fonts/FreeSerifBoldItalic18pt7b.h> 
//#include <Fonts/FreeSerifBold12pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include "bitmaps.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <WiFi.h>
#include <WiFiClient.h>

const char* ssid = "DT_LAB";
const char* password = "fthu@050318";

// For the breakout, you can use any 2 or 3 pins
// These pins will also work for the 1.8" TFT shield
#define TFT_CS 16
#define TFT_RST 9  // you can also connect this to the Arduino reset
                      // in which case, set this #define pin to -1!
#define TFT_DC 17

// Option 1 (recommended): must use the hardware SPI pins
// (for UNO thats sclk = 13 and sid = 11) and pin 10 must be
// an output. This is much faster - also required if you want
// to use the microSD card (see the image drawing example)
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

// Option 2: use any pins but a little slower!
#define TFT_SCLK 5   // set these to be whatever pins you like!
#define TFT_MOSI 23   // set these to be whatever pins you like!
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Adafruit IO
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "siddheshCG"
#define AIO_KEY         "5e0217bf2f53446983193bc050996163"  // Obtained from account info on io.adafruit.com


//colors
#define gray_color 0x18A8
#define bisleri_color 0x0450
#define dettol_color 0x0be7
//load cell parameters
#define calibration_factor -260000 //This value is obtained using the SparkFun_HX711_Calibration sketch

#define DOUT  33
#define CLK  27
#define kgToLbs 2.2046226218 
HX711 scale;

byte count = 0;
byte connectMqtt =0;
float weightLBS=0;
float weightKG;
float prevWeightKG;
float weightGRAMS;
String prevWgt;
String weightInKg;


#define bottle_weight 275.00
#define soap_weight 75
#define offset 10.00

int bottle_stock = 0;
int prev_bottle_stock=0;
int new_bottle_stock = 0;
int prev_new_bottle_stock = 1;
byte soap_stock = 4;
byte prev_soap_stock = 0;
bool stock_updated = false;
unsigned long currentMillis = 0;
unsigned long prevoiousMillis = 0;

//////////////////////
// Button Variables //
//////////////////////
int BUTTON_STATE;
int LAST_BUTTON_STATE = HIGH;
long LAST_DEBOUNCE_TIME = 0;
long DEBOUNCE_DELAY = 3000;
int BUTTON_COUNTER = 0;
int IDLE_DELAY=5000;
//int lastPressedMillis=0;
int middleButton = 34;
int upButton = 39;
int downButton = 35;
bool setting = false;
bool settingMode = false;
bool settingPage1 = false;
bool settingPage2 = false;
bool check_stock = false;
WiFiClient client;
 
// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
 
/****************************** Feeds ***************************************/

Adafruit_MQTT_Publish smart_shelf_2 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/smart_shelf_2");
Adafruit_MQTT_Publish stock_update = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/stock_update");
 
void setup() {
  Serial.begin(9600);
  pinMode(middleButton, INPUT);
  pinMode(upButton, INPUT);
  pinMode(downButton, INPUT);
  Serial.println("HX711 scale demo");
  // Use this initializer if you're using a 1.8" TFT
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab

  // Use this initializer (uncomment) if you're using a 1.44" TFT
  //tft.initR(INITR_144GREENTAB);   // initialize a ST7735S chip, black tab

  // Use this initializer (uncomment) if you're using a 0.96" 180x60 TFT
  //tft.initR(INITR_MINI160x80);   // initialize a ST7735S chip, mini display
  //tft.setFont(&FreeSerifBoldItalic18pt7b);  
  tft.setFont(&FreeSerif9pt7b);
  tft.setRotation(1);
  //u8g2.setFont(u8g2_font_timB24_tn); 
  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor); //This value is obtained by using the SparkFun_HX711_Calibration sketch
  scale.tare(); //Assuming there is no weight on the scale at start up, reset the scale to 0

  Serial.println("Readings:");
  delay(2000);
  //-426720 zero factor 
  /*tft.fillScreen(ST7735_WHITE);
  bisleriMRP_page();
  
  delay(2000);
  tft.fillScreen(ST7735_WHITE);
  dettolMRP_page();
  delay(2000);
  tft.fillScreen(ST7735_WHITE);
  dettol_offer_page();*/
  tft.fillScreen(ST7735_WHITE);
  for(int i=0; i<5; i++){
    weightLBS += scale.get_units();
  }
  weightLBS /=5;
  weightKG = weightLBS / kgToLbs;
  weightGRAMS = weightKG*1000;
  prevWeightKG = weightGRAMS;
  //prev_bottle_stock = bottle_stock;
  delay(1000);
  Serial.print("current: ");
  Serial.println(weightGRAMS);
  Serial.print("previous: ");
  Serial.println(prevWeightKG);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println();

  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());
  // connect to adafruit io 
  mqtt.connect();
  currentMillis = millis();
  prev_bottle_stock = bottle_stock;
  tft.fillScreen(ST7735_WHITE);
    bisleriMRP_page();
    delay(3000);
    tft.fillScreen(ST7735_WHITE);
 }

void bisleriMRP_page(){
    //Case 2: Multi Colored Images/Icons
  int h = 128,w = 93, row, col, buffidx=0;
  for (row=0; row<h; row++) { // For each scanline...
    for (col=0; col<w; col++) { // For each pixel...
      //To read from Flash Memory, pgm_read_XXX is required.
      //Since image is stored as uint16_t, pgm_read_word is used as it uses 16bit address
      tft.drawPixel(col, row, pgm_read_word(bottle + buffidx));
      buffidx++;
    } // end pixel
  }
  
 tft.fillRect(90, 45 , 60,35, bisleri_color);
 testdrawtext(98, 72, "Rs.6", ST7735_WHITE); 
}

void bisleriOutOfStock_page(){
   int h = 128,w = 93, row, col, buffidx=0;
  for (row=0; row<h; row++) { // For each scanline...
    for (col=0; col<w; col++) { // For each pixel...
      //To read from Flash Memory, pgm_read_XXX is required.
      //Since image is stored as uint16_t, pgm_read_word is used as it uses 16bit address
      tft.drawPixel(col, row, pgm_read_word(bottle + buffidx));
      buffidx++;
    } // end pixel
  }
  
 tft.fillRect(90, 45 , 60,35, bisleri_color);
 testdrawtext(98, 72, "Rs.6", ST7735_WHITE);
 //tft.fillRect(60, 80 , 100,35, bisleri_color);
 testdrawtext(100, 100, "Out Of", ST7735_RED);
 testdrawtext(110, 120, "Stock", ST7735_RED); 
}
void dettolMRP_page(){
  int h1 = 100,w1 = 100, row1, col1, buffidy=0;
  for (row1=-10; row1<h1; row1++) { // For each scanline...
    for (col1=((tft.width()/2)-50); col1<w1+((tft.width()/2)-50); col1++) { // For each pixel...
      //To read from Flash Memory, pgm_read_XXX is required.
      //Since image is stored as uint16_t, pgm_read_word is used as it uses 16bit address
      tft.drawPixel(col1, row1, pgm_read_word(dettol + buffidy));
      buffidy++;
    } // end pixel
  }
  tft.fillRect(((tft.width()/2)-50), 75 , 100,35, dettol_color);
  testdrawtext(((tft.width()/2)-40), 102, "Rs.30", ST7735_BLUE);
}

void dettol_offer_page(){
  int h1 = 128,w1 = 109, row1, col1, buffidy=0;
  for (row1=0; row1<h1; row1++) { // For each scanline...
    for (col1=((tft.width()/2)-50); col1<w1+((tft.width()/2)-50); col1++) { // For each pixel...
      //To read from Flash Memory, pgm_read_XXX is required.
      //Since image is stored as uint16_t, pgm_read_word is used as it uses 16bit address
      tft.drawPixel(col1, row1, pgm_read_word(dettol_offer + buffidy));
      buffidy++;
    } // end pixel
  }
}

void stock_track_page(){
//  tft.setFont(&FreeSerifBold12pt7b); 
  testdrawtext(0, 55, "Bisleri: ", ST7735_BLUE);
  if(bottle_stock != prev_bottle_stock){
    testdrawtext(0, 55, String(bottle_stock), ST7735_BLUE);
    testdrawtext(0, 55, String(bottle_stock), ST7735_BLUE);
  }
  else
    testdrawtext(0, 55, String(bottle_stock), ST7735_BLUE);

}
void testdrawtext(int a, int b, String text, uint16_t color) {
  tft.setCursor(a, b);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
  
}

void setting_page1(){
   if(setting == true){
      tft.fillScreen(ST7735_BLACK);
      // tft.fillCircle(4, 17, 3, ST7735_RED);
      // tft.fillCircle(4, 47, 3, ST7735_RED);
      tft.fillRect(5, 25, tft.width(), 25, ST7735_BLUE);
      testdrawtext(10, 42, "Update Bottle Stock", ST7735_YELLOW);
      tft.fillRect(tft.width()-50, tft.height()-30 , 50, 25, ST7735_BLUE);
      testdrawtext(tft.width()-40, tft.height()-13, "Back", ST7735_YELLOW);
    }
    int upCounter = digitalRead(upButton);
    int downCounter = digitalRead(downButton);
    if(upCounter == LOW){
      settingPage2 = true;
      Serial.println("setting page 2");
      settingPage1 = false;
    }
    if(downCounter == LOW){
      settingMode = false;
      Serial.println("setting page 1");
    }
  
}

void setting_page2(){
  int up = digitalRead(upButton);
  int down = digitalRead(downButton);
  int select = digitalRead(middleButton);
  if(up == LOW){
    new_bottle_stock+=5;
    Serial.println(new_bottle_stock);
    delay(250);
  }
  if(down == LOW){
    new_bottle_stock-=5;
    Serial.println(new_bottle_stock);
    delay(250);
  }
  if(select == LOW && stock_updated == false){
    check_stock = true;
    tft.fillScreen(ST7735_BLACK);
    String msg = "Adding " + String(new_bottle_stock) + " bottles";
    testdrawtext(0, 50, msg, ST7735_YELLOW);
    Serial.println(msg);
    delay(3000);
    tft.fillScreen(ST7735_BLACK); 
  }
  if(check_stock == true){
    Serial.println("check weight of bottles");
    for(int i=0; i<5; i++){
        weightLBS += scale.get_units();
     }
    weightLBS /=5;
    weightKG = weightLBS / kgToLbs;
    weightGRAMS = weightKG*1000;
    prevWeightKG = weightGRAMS;
    Serial.println(int(weightGRAMS));
    Serial.println(weightGRAMS / 275);
    Serial.println((float(new_bottle_stock)-0.3));
    if(((weightGRAMS / 275) >= (float(new_bottle_stock)-0.3)) && ((weightGRAMS / 275) <= (float(new_bottle_stock)+0.3))){
       bottle_stock += new_bottle_stock;
       prev_bottle_stock = bottle_stock;
       updateDashboard();   
       tft.fillScreen(ST7735_WHITE);
       settingMode = false;
       settingPage2 = false;
       check_stock = false;
       stock_updated = true;
    }
   else{
     tft.fillScreen(ST7735_BLACK);
    testdrawtext(0, 50, "Stock Update Failed", ST7735_RED);
    testdrawtext(0, 80, "Try Again", ST7735_RED);
    delay(3000);
    tft.fillScreen(ST7735_WHITE);
    settingMode = false;
       settingPage2 = false;
       check_stock == false;
   }
  }
  if(new_bottle_stock != prev_new_bottle_stock){
     tft.fillScreen(ST7735_BLACK);
     tft.fillTriangle(145, 25, 135, 45, 155, 45, ST7735_BLUE);
     testdrawtext(130, 80, String(new_bottle_stock), ST7735_YELLOW);
     tft.fillTriangle(145, tft.height()-5, 135, tft.height()-25, 155, tft.height()-25, ST7735_BLUE);
     prev_new_bottle_stock=new_bottle_stock;
   }
}

void updateDashboard(){
   // ping adafruit io a few times to make sure we remain connected
    if(! mqtt.ping(3)) {
      // reconnect to adafruit io
      if(! mqtt.connected())
        mqtt.connect();
    }
   delay(100);
    if (! stock_update.publish(new_bottle_stock))
        Serial.println(F("Failed to publish"));
     else
        Serial.println(F("published!"));
     Serial.print("Stock updated to dashboard");
  delay(10);   
   if (! smart_shelf_2.publish(bottle_stock))
        Serial.println(F("Failed to publish"));
     else
        Serial.println(F("published!"));
     Serial.print("Stock updated to dashboard");
}
// Debounce Button Presses
boolean debounce() {
  boolean retVal = false;
  int reading = digitalRead(middleButton);
  Serial.println(reading);
  if (reading == LOW && LAST_BUTTON_STATE != reading) {
    LAST_DEBOUNCE_TIME = millis();
    reading = BUTTON_STATE;
    LAST_BUTTON_STATE = reading;
  }
  if ((millis() - LAST_DEBOUNCE_TIME) > DEBOUNCE_DELAY) {
    if (reading == BUTTON_STATE) {
      BUTTON_STATE = reading;
      if (BUTTON_STATE == LOW) {
        retVal = true;
        settingPage1 = true;
        settingMode = true;
        LAST_BUTTON_STATE = HIGH;
      }
    }
  }
  //LAST_BUTTON_STATE = reading;
  return retVal;
}


void loop() {
  
 /* tft.fillScreen(ST7735_WHITE);
  bisleriMRP_page();
  delay(1000);
  tft.fillScreen(ST7735_WHITE);
  dettolMRP_page();
  delay(1000);
  tft.fillScreen(ST7735_WHITE);
  dettol_offer_page();
  delay(1000);
  tft.fillScreen(ST7735_WHITE);*/
   if(connectMqtt >= 9){
    Serial.println("connecting");
    // ping adafruit io a few times to make sure we remain connected
    if(! mqtt.ping(3)) {
      // reconnect to adafruit io
      if(! mqtt.connected())
        mqtt.connect();
    }
   connectMqtt = 0;
  }
  setting = debounce();
  if(settingMode == true){
    Serial.println("setting mode");
    if(settingPage1 == true){
       setting_page1();
     }
    if(settingPage2 == true){
       setting_page2();
     }
   if(settingMode == false){
    tft.fillScreen(ST7735_WHITE);
   }
  }
 
  if(settingMode == false){
    
   prevWgt = weightInKg;
  
  for(int j=0; j<3;j++){
  if((prevWeightKG != weightGRAMS) && (((prevWeightKG - weightGRAMS)> 10) ||((weightGRAMS - prevWeightKG)> 10))) count++;
  if(count >= 2){
    count = 0;
    prevWeightKG = weightGRAMS;
  }
  for(int i=0; i<5; i++){
    weightLBS += scale.get_units();
  }
  weightLBS /=5;
  weightKG = weightLBS / kgToLbs;
  weightGRAMS = weightKG*1000;
  weightInKg = String(weightKG)+"KG";
  /*Serial.print("Weight: ");
  Serial.print(weightLBS); //scale.get_units() returns a float
  Serial.print("lbs  "); //You can change this to kg but you'll need to refactor the calibration_factor
  Serial.print(weightKG);
  Serial.print("KG");
  Serial.print("  ");
  Serial.print(weightGRAMS);
  Serial.println("Gram");*/
  weightLBS = 0;
  connectMqtt++;
  delay(250);
  stock_track(weightGRAMS, prevWeightKG);   
  }
  Serial.println();
  Serial.println((millis() - currentMillis));
  //stock_track_page();
  if((bottle_stock != prev_bottle_stock) && ((millis() - currentMillis) >10000)){
  if (! smart_shelf_2.publish(bottle_stock))
        Serial.println(F("Failed to publish"));
     else
        Serial.println(F("published!"));
     Serial.print("Stock updated to dashboard");
    prev_bottle_stock = bottle_stock; 
  }
 if(bottle_stock == 0 && stock_updated == true){
    if (! stock_update.publish(bottle_stock))
        Serial.println(F("Failed to publish"));
     else
        Serial.println(F("published!"));
     Serial.print("Stock updated to dashboard");
     stock_updated = false;
     tft.fillScreen(ST7735_WHITE);
 }
 if(bottle_stock == 0){
  //tft.fillScreen(ST7735_WHITE);
    bisleriOutOfStock_page();
 }
 else{
  //tft.fillScreen(ST7735_WHITE);
    bisleriMRP_page();
 }
 }
}


void stock_track(float currentWeight, float prevWeight){
  Serial.print("current: ");
  Serial.println(currentWeight);
  Serial.print("previous: ");
  Serial.println(prevWeight);
  Serial.print("difference: ");
  Serial.println(prevWeight - currentWeight); 
  
  if(((prevWeight - currentWeight) > (bottle_weight-offset))) {//&& ((prevWeight - currentWeight) < (bottle_weight+offset))){
    bottle_stock = bottle_stock - int((((prevWeight - currentWeight)+15) / bottle_weight));
    Serial.print("Bottle Removed: ");
    Serial.println(int(round(((prevWeight - currentWeight)+15) / bottle_weight)));
    prevWeightKG = weightGRAMS;
    currentMillis = millis();
  }
  if(((currentWeight - prevWeight) > (bottle_weight-offset))){// && ((currentWeight - prevWeight) < (bottle_weight+offset))){
    bottle_stock = bottle_stock + int(((currentWeight - prevWeight)+15) / bottle_weight);
    Serial.print("Bottle removed: ");
    Serial.println(int(round(((currentWeight - prevWeight)+15) / bottle_weight)));
    prevWeightKG = weightGRAMS;
    currentMillis = millis();
  }
  Serial.print("Bottle stock: ");
  Serial.println(bottle_stock);
  Serial.print("Prev Bottle stock: ");
  Serial.println(prev_bottle_stock);
  //delay(500);

}
