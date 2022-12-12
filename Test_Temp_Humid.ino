#include <SD.h>
#include <DHT.h>
#include <SPI.h>
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define SEALEVELPRESSURE_HPA (1013.25)
#define TCAADDR 0x70

// tetapkan id unik untuk setiap sensor bme dan RTC
Adafruit_BME280 bme1;
Adafruit_BME280 bme2;
tmElements_t tm;
const int chipSelect = 4;

// menentukan pin arduino mana yang digunakan untuk menerima data sensor dht
#define DHT1PIN  3 // pin digital untuk DHT11_1
#define DHT2PIN  5 // pin digital untuk DHT11_2
#define DHT3PIN  6 // pin digital untuk DHT22_1
#define DHT4PIN  7 // pin digital untuk DHT22_2
// mendefinisikan jenis sensor dht yang digunakan
#define DHT1TYPE DHT11 // DHT 11_1
#define DHT2TYPE DHT11 // DHT 11_2
#define DHT3TYPE DHT22 // DHT 22_1
#define DHT4TYPE DHT22 // DHT 22_2
// menugaskan nama, pin, dan jenis setiap sensor dht
DHT dht1(DHT1PIN, DHT1TYPE);
DHT dht2(DHT2PIN, DHT2TYPE);
DHT dht3(DHT3PIN, DHT3TYPE);
DHT dht4(DHT4PIN, DHT4TYPE);

int nr = 1;
File dataFile;

// fungsi multiplekser I2C
void tcaselect(uint8_t i)
{
 if (i > 7) return;
 Wire.beginTransmission(TCAADDR);
 Wire.write(1 << i);
 Wire.endTransmission();
}

void setup()
{
 Serial.begin(9600);
 Serial.print("Initializing SD card...");
 // lihat apakah kartu ada dan dapat diinisialisasi
 if (!SD.begin(chipSelect)) 
 {
  Serial.println("Card failed, or not present");
  // jangan lakukan hal lain
  while (1);
 }
 Serial.println("card initialized.");
 // menginisialisasi bme280 pertama
 tcaselect(7);
 if (!bme1.begin(0x76))
 {
 // Terjadi masalah saat mendeteksi bme1 ... periksa koneksi Anda
 Serial.println("Ooops, no bme1 detected ... Check your wiring!");
 }
 // initialize the second bme280:
 tcaselect(4);
 if (!bme2.begin(0x76))
 {
 // There was a problem detecting the bme1 ... check your connections
 Serial.println("Ooops, no bme2 detected ... Check your wiring!");
 }
 // aktifkan sensor dht dan periksa apakah berfungsi
 dht1.begin();
 dht2.begin();
 dht3.begin();
 dht4.begin();
}

void loop()
{
 Serial.println("Inside loop");
 tcaselect(2);
 RTC.read(tm);

 int Day    = tm.Day;
 int Month  = tm.Month;
 int Year   = tmYearToCalendar(tm.Year);
 int Hour   = tm.Hour;
 int Minute = tm.Minute;
 int Second = tm.Second;
tcaselect(7);
 int h5     = bme1.readHumidity()*100;
 int t5     = bme1.readTemperature()*100;
 tcaselect(4);
 int h6     = bme2.readHumidity()*100;
 int t6     = bme2.readTemperature()*100;
 // membuat variabel untuk membaca T(C), RH(%), P(hPa) dan Alt(m)
 float h1   = dht1.readHumidity();
 float t1   = dht1.readTemperature(); // baca dht11_1
 float h2   = dht2.readHumidity();
 float t2   = dht2.readTemperature(); // baca dht11_2
 float h3   = dht3.readHumidity();
 float t3   = dht3.readTemperature(); // baca dht22_1
 float h4   = dht4.readHumidity();
 float t4   = dht4.readTemperature(); // baca dht22_2
 // membuat string
 String dataString1 = 
    String(Day)    +"/"+ 
    String(Month)  +"/"+ 
    String(Year)   +","+ 
    String(Hour)   +":"+ 
    String(Minute) +":"+ 
    String(Second) +",";
 String dataString2 = 
    String(h1)     +","+
    String(t1)     +","+
    String(h2)     +","+
    String(t2)     +",";
 String dataString3 = 
    String(h3)     +","+
    String(t3)     +","+
    String(h4)     +","+
    String(t4);
 String dataString4 = 
    String(h5)     +","+
    String(t5)     +","+
    String(h6)     +","+
    String(t6);

 // buka file. perhatikan bahwa hanya satu file yang dapat dibuka pada satu waktu
 // jadi kamu harus menutup yang ini sebelum membuka yang lain
 File dataFile1 = SD.open("datalog1.csv", FILE_WRITE);
 // jika file tersedia, tulislah
 if (dataFile1)
 {
  dataFile1.print(dataString1);
  dataFile1.print(dataString2);
  dataFile1.println(dataString3);
  dataFile1.close();
 }
 // jika file tidak terbuka, pop up error
 else 
 {
  Serial.println("error opening datalog1.csv");
 }
 delay(1000);

 File dataFile2 = SD.open("datalog2.csv", FILE_WRITE);
 // jika file tersedia, tulislah
 if (dataFile2)
 {
  dataFile2.print(dataString1);
  dataFile2.println(dataString4);
  dataFile2.close();
 }
 // jika file tidak terbuka, pop up error
 else
 {
  Serial.println("error opening datalog2.csv");
 }
 delay(57500);
}
