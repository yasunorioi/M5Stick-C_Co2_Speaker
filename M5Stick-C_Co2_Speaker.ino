/*
 * M5Stack SCD40 CO2 Unit
 * M5StickC Speaker Hat（PAM8303)
*/
#include <M5StickC.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>

const int servo_pin = 26;
int freq            = 50;
int ledChannel      = 0;
int resolution      = 10;
const int16_t SCD_ADDRESS = 0x62;
float co2_alert = 1000;

uint32_t chipId = 0;

byte host[] = {192, 168, 1, 23};
int port = 8089;

WiFiClient client;
WiFiUDP udp;


//Ambient ambient;

const char* ssid = "SSID";         // WiFi SSID
const char* password = "pass";     // WiFi パスワード

extern const unsigned char m5stack_startup_music[];

void setup() {
    // put your setup code here, to run once:
    M5.begin(true,false,true);
    pinMode(0,OUTPUT);
    digitalWrite(0,LOW);
    M5.Lcd.setRotation(3);
    M5.Lcd.setCursor(0, 0, 4);
    M5.Lcd.setTextFont(2);
    //M5.Lcd.println("speaker");
    M5.Axp.ScreenBreath(8);
    
    // init I2C
    Wire.begin();

    // wait until sensors are ready, > 1000 ms according to datasheet
    delay(1000);
    
    // start scd measurement in periodic mode, will update every 5 s
    Wire.beginTransmission(SCD_ADDRESS);
    Wire.write(0x21);
    Wire.write(0xb1);
    Wire.endTransmission();

    // wait for first measurement to be finished
    delay(5000);
    
    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(servo_pin, ledChannel);
    ledcWrite(ledChannel, 256);  // 0°
    WiFi.begin(ssid, password);              //  Wi-Fi APに接続
      while (WiFi.status() != WL_CONNECTED) {  //  Wi-Fi AP接続待ち
    delay(500);
    }
    Serial.print("WiFi connected\r\nIP address: ");
    Serial.println(WiFi.localIP());    

  Serial.print("WiFi connected\r\nIP address: ");
  Serial.println(WiFi.localIP()); 
    for(int i=0; i<17; i=i+8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
}
void playMusic(const uint8_t* music_data, uint16_t sample_rate) {
    uint32_t length         = strlen((char*)music_data);
    uint16_t delay_interval = ((uint32_t)1000000 / sample_rate);
    for (int i = 0; i < length; i++) {
        ledcWriteTone(ledChannel, music_data[i] * 50);
        delayMicroseconds(delay_interval);
    }
}
void loop() {
    // put your main code here, to run repeatedly:
    float co2, temperature, humidity;
    uint8_t data[12], counter;
    
    // send read data command
    Wire.beginTransmission(SCD_ADDRESS);
    Wire.write(0xec);
    Wire.write(0x05);
    Wire.endTransmission();

    // read measurement data: 2 bytes co2, 1 byte CRC,
    // 2 bytes T, 1 byte CRC, 2 bytes RH, 1 byte CRC,
    // 2 bytes sensor status, 1 byte CRC
    // stop reading after 12 bytes (not used)
    // other data like  ASC not included
    Wire.requestFrom(SCD_ADDRESS, 12);
    counter = 0;
    while (Wire.available()) {
        data[counter++] = Wire.read();
    }

    // floating point conversion according to datasheet
    co2 = (float)((uint16_t)data[0] << 8 | data[1]);
    // convert T in degC
    temperature = -45 + 175 * (float)((uint16_t)data[3] << 8 | data[4]) / 65536;
    // convert RH in %
    humidity = 100 * (float)((uint16_t)data[6] << 8 | data[7]) / 65536;
    /*
     * Send InfluxDB
     */
    String line = "SDB40 device="+String(chipId)+",Co2="+String(co2,0)+",temperature="+String(temperature)+",humidity="+String(humidity);
    Serial.println(line);
    udp.beginPacket(host, port);
    udp.print(line);
    udp.endPacket();
    delay(100);
    M5.Lcd.setCursor(0, 0, 4);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE); 
    M5.Lcd.println("Co2:"+String(co2,0));
    M5.Lcd.println("temp:"+String(temperature));
    M5.Lcd.println("humi:"+String(humidity));
    
    if (co2>co2_alert){
    M5.Lcd.fillScreen(RED);
    M5.Lcd.setTextColor(BLACK); 
    ledcWriteTone(ledChannel, 1250);
    delay(1000);
    ledcWriteTone(ledChannel, 0);
    delay(1000);
} else {
  ledcWriteTone(ledChannel, 0);
}
    
    delay(5000);
}
