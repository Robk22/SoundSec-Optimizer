
// Source: https://eu-central-1-1.aws.cloud2.influxdata.com/orgs/c1189275562d716a/load-data/client-libraries/arduino
#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define SoundSensorPin A0  //this pin read the analog voltage from the sound level meter
#define VREF  3.0  //voltage on AREF pin,default:operating voltage

// WiFi AP SSID
#define WIFI_SSID "xxxxxxxxxxx"
// WiFi password
#define WIFI_PASSWORD "xxxxxxxxxxx"
// InfluxDB v2 server url
#define INFLUXDB_URL "xxxxxxxxxxx"
// InfluxDB v2 server or cloud API authentication token (Use: InfluxDB UI -> Data -> Tokens -> <select token>)
#define INFLUXDB_TOKEN "xxxxxxxxxxx"
// InfluxDB v2 organization id
#define INFLUXDB_ORG "xxxxxxxxxxx"
// InfluxDB v2 bucket name
#define INFLUXDB_BUCKET "xxxxxxxxxxx"

// Set timezone for Central Europe
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Data point
Point sensor("db_status");

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(2,13,NEO_GRBW + NEO_KHZ800);
 
U8G2_SH1107_SEEED_128X128_1_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);
 
const int pinAdc = A0;

int ledPin = 12;
 
void setup()
{   
  Serial.begin(115200);

  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  // Add tags
  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());

  // Accurate time is necessary for certificate validation and writing in batches
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
  
    // Initialize Neopixel
    pixels.begin();
    delay(1);
    pixels.setPixelColor(0,0,50,0,0);
    pixels.show();
     
    // initialize digital pin12  as an output.
    pinMode(ledPin, OUTPUT);
    u8g2.begin();

}
 
void loop()
{    
    float dB = 0;
    String recommendation;
    u8g2.clearBuffer();

    //Get the average dB of the last 10 intervals
    for (int i = 0; i<10; i++){
      dB += voltageToDecibel();
    }
    dB /= 10;

    //Green
    if(dB < 60) {
      digitalWrite(ledPin, HIGH);
      recommendation = "";
      pixels.setPixelColor(0,0,20,0,0);
      pixels.show();
    } else if (dB < 80) { //Yellow
      digitalWrite(ledPin, LOW);
      recommendation = "";
      pixels.setPixelColor(0,20,20,0,0);
      pixels.show();
    } else { //Red
      digitalWrite(ledPin, LOW);
      recommendation = "Leiser!";
      pixels.setPixelColor(0,20,0,0,0);
      pixels.show();
    }
    Serial.println(dB);

    //Output to OLED Display
     u8g2.firstPage();
     String dBstring; 
  do {
    u8g2.setFont(u8g2_font_ncenB18_tf);
    dBstring = String(dB);
    u8g2.setCursor(0, 30);
    u8g2.print("Loudness:");
    u8g2.setCursor(5, 70);
    u8g2.print(dBstring + " dB");
    u8g2.setCursor(5, 100);
    u8g2.print(recommendation);
  } while ( u8g2.nextPage() );

  // Clear fields for reusing the point. Tags will remain untouched
  sensor.clearFields();

  // Store measured value into point
  sensor.addField("dB", dB);

  // Print what are we exactly writing
  Serial.print("Writing: ");
  Serial.println(sensor.toLineProtocol());

  // If no Wifi signal, try to reconnect it
  if ((WiFi.RSSI() == 0) && (wifiMulti.run() != WL_CONNECTED)) {
    Serial.println("Wifi connection lost");
  }

  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

}

float voltageToDecibel() 
 {
    float voltageValue,dBValue;
    voltageValue = analogRead(SoundSensorPin) / 1024.0 * VREF;
    dBValue = voltageValue * 50.0;  //convert voltage to decibel value
    Serial.print(dBValue,1);
    Serial.println(" dBA"); 
    return dBValue;
 } 
