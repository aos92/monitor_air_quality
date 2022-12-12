#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <LiquidCrystal_I2C.h>
#define SEALEVELPRESSURE_HPA (1013.25)
#define TCAADDR 0x70

//// tetapkan id unik ke sensor bme dan RTC ////
Adafruit_BME280 bme1;
tmElements_t tm;

//// untuk berkomunikasi dengan kartu SD ////
const int chipSelect = 4;
// penghitung untuk pencatatan data
int nr = 1;
// file untuk mencatat data
File dataFile;

//// tetapkan alamat LCD I2C ////
LiquidCrystal_I2 lcd(0x27, 16,2);

//// atur pin LED RGB ////
int redpin = 2; // pin untuk LED merah
int bluepin = 3; // pin untuk LED biru
int greenpin = 7; // pin untuk LED hijau
int val;

//// penghitung dan larik untuk penyimpanan 3 nilai NH3 ////
int i = 0;
int mySensValsNH3[3];
int mySensValsh1[3];
int mySensValst1[3];
int mySensValsAlt1[3];
int NH3mean = 0;
int h1mean = 0;
int t1mean = 0;
int Alt1mean = 0;
int ppmNH3calibrround;
int sumNH3 = 0;
int sumh1 = 0;
int sumt1 = 0;
int sumAlt1 = 0;

//// TCA9548A I2C fungsi multiplekser ////
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
 lcd.begin(16, 2);
 pinMode(redpin, OUTPUT);
 pinMode(bluepin, OUTPUT);
 pinMode(greenpin, OUTPUT);
 Serial.print("Initializing SD card...");

 // lihat apakah kartu ada dan dapat diinisialisasi
 if (!SD.begin(chipSelect)) 
 {
  Serial.println("Card failed, or not present");
 // jangan lakukan hal lain
  while (1);
 }

 Serial.println("card initialized.");
 File dataFile = SD.open("datalog1.csv", FILE_WRITE);

 if (dataFile) 
 {
  dataFile.println(" , , , , , ,"); // baris untuk memisahkan kumpulan data
  String Header = "Date, Time, RH[%], T[oC], Height[m], [NH3]";
        
  dataFile.println(Header);
  dataFile.close();
  Serial.println(Header);
 }
 else 
 {
  Serial.println("Couldn't open log file");
 }
    
 //// menginisialisasi sensor BME280 ////
 tcaselect(4);
 if (!bme1.begin(0x76)) 
 {
  // ada masalah saat mendeteksi bme1 ... periksa koneksi Anda
  Serial.println("Ooops, no bme1 detected ... Check your wiring!");
 }
 // waktu pemanasan...
 Serial.println("Please wait...Preparing system.");
 // tulis ke LCD
 tcaselect(2);
 // atur kursor ke (0,0)
 lcd.setCursor(0,0);
 lcd.print("Please wait...");

 //// atur kursor ke (16,1) ////
 lcd.setCursor(0,1);
 lcd.print("Preparing system");
 delay(900000); // waktu pemanasan resistansi MQ137
 Serial.println("Ready!!!");
 lcd.setCursor(0,0);
 lcd.print("Ready!!! ");
 lcd.setCursor(0,1);
 lcd.print(" ");
}

void loop()
{
 const int MQ137_PIN = A3;
 const float RL_MQ137 = 1.0; // kohm (modul FC-22 = 1,0 kohm)
 const float CLEAN_AIR_RATIO_MQ137 = 3.6; // diambil dari grafik lembar data
 float sensorValue_MQ137;
 float sensor_volt_MQ137;
 float Rs_MQ137; // kohm
 const float R0_MQ137 = 16.10; // kohm , dari sketsa pencari R0
 float AirFraction_MQ137;
 float AirFractionMin_MQ137 = 0.118851;
 float AirFractionMax_MQ137 = 0.388153;
 float AirFractionCurva33_MQ137;
 float AirFractionCurva85_MQ137;
 float AirFractionCalibr_MQ137;

 //// menghitung Rs dari data analog ////
 sensorValue_MQ137 = analogRead(A3);
 sensor_volt_MQ137 = sensorValue_MQ137 * (5.0 / 1023.0); // ubah ke tegangan
 Rs_MQ137 = ((5.0 * RL_MQ137) / sensor_volt_MQ137) - RL_MQ137; // hitung Rs
 AirFraction_MQ137 = Rs_MQ137/R0_MQ137;
    
 //// dapatkan tanggal dan waktu RTC ////
 tcaselect(7);
 RTC.read(tm);
 int Day = tm.Day;
 int Month = tm.Month;
 int Year = tmYearToCalendar(tm.Year);
 int Hour = tm.Hour;
 int Minute = tm.Minute;
 int Second = tm.Second;

 //// dapatkan data sensor BME280 ////
 tcaselect(4);
 float h1 = bme1.readHumidity() + 6.89;
 int h1round = h1;
 float t1 = bme1.readTemperature()+0.55;
 int t1round = t1;
 float Alt1 = bme1.readAltitude(1013.25) + 45;
 int Alt1round = Alt1;
 float ppmNH3calibr;
 float ppmNH3;

 // kalibrasi MQ137 dengan mempertimbangkan Suhu dan kelembapan relatif
 // berdasarkan lembar data Hanwen, menggunakan regresi polinomial
 AirFractionCurva33_MQ137 = 0.0003 * pow(t1, 2) - 0.0255 * t1 + 1.4004;
 AirFractionCurva85_MQ137 = 0.0003 * pow(t1, 2) - 0.0231 * t1 + 1.2808;

 if (h1 >= 0 && h1 < 59) 
 {
 AirFractionCalibr_MQ137 = AirFraction_MQ137 / AirFractionCurva33_MQ137;
 }
 else
 {
  AirFractionCalibr_MQ137 = AirFraction_MQ137 / AirFractionCurva85_MQ137;
 }

 //// Konversi ke ppm dari NH3 ////
 float expoente_MQ137 = 1 / (-0.257);
 ppmNH3 = pow((AirFraction_MQ137 / 0.587), expoente_MQ137);
 ppmNH3calibr = pow((AirFractionCalibr_MQ137 / 0.587), expoente_MQ137);
 // membulatkan [NH3] ke int lalu memasukkan nilai [NH3], RH dan T ke dalam array masing-masing

 for (i=0; i<=2; i=i+1)
 {
  ppmNH3calibrround=round(ppmNH3calibr);
  mySensValsNH3[i] = ppmNH3calibrround;
  sumNH3 = sumNH3 + mySensValsNH3[i];
  mySensValsh1[i] = h1round;
  sumh1 = sumh1 + mySensValsh1[i];
  mySensValst1[i] = t1round;
  sumt1 = sumt1 + mySensValst1[i];
  mySensValsAlt1[i] = Alt1round;
  sumAlt1 = sumAlt1 + mySensValsAlt1[i];

  if(i==2)
  {
   // rata-rata dari 3 nilai terakhir yang diukur
   NH3mean = sumNH3 / 3;
   h1mean = sumh1 / 3;
   t1mean = sumt1 / 3;
   Alt1mean = sumAlt1 / 3;
   tcaselect(2);

   lcd.setCursor(0,0);
   lcd.print("T:");
   lcd.print(t1mean);
   lcd.print("oC ");
   lcd.print("RH:");
   lcd.print(h1mean);
   lcd.print("% ");
   lcd.setCursor(0,1);
   lcd.print("NH3:");
   lcd.print(NH3mean);
   lcd.print("ppm ");
  }

  // ubah warna LED RGB menurut [NH3]
  if (NH3mean >= 5 && NH3mean <= 200)
  {
   if (NH3mean <= 25)
   {
    analogWrite(greenpin, 150);
    analogWrite(bluepin, 0);
    analogWrite(redpin, 0);
   } 
   else 
   {
    if (NH3mean <= 80) 
    {
     analogWrite(bluepin, 150);
     analogWrite(greenpin, 0);
     analogWrite(redpin, 0);
    }
    else 
    {
     analogWrite(redpin, 150);
     analogWrite(greenpin, 0);
     analogWrite(bluepin, 0);
    }
   }
  }
  else 
  {
   if (NH3mean < 5) 
   {
    lcd.setCursor(0,1);
    lcd.print("[NH3] OutOfRange");
    analogWrite(greenpin, 150);
    analogWrite(redpin, 0);
    analogWrite(bluepin, 0);
  }
  else 
  {
   lcd.setCursor(0,1);
   lcd.print("[NH3] OutOfRange");
   analogWrite(redpin, 150);
   analogWrite(greenpin, 0);
   analogWrite(bluepin, 0);
  }
 }
  delay(1000);
 }

 //// membuat string ////
 String dataString1 = String(Day) + "/" + String(Month) + "/" + String(Year) + "," + String(Hour) + ":" + String(Minute) + ":" + String(Second) + ",";
 String dataString2 = String(h1mean) + "," + String(t1mean) + "," + String(Alt1mean) + "," + String(NH3mean);

 // buka file (hanya satu per satu jadi Anda harus menutup yang ini sebelum membuka yang lain
 File dataFile1 = SD.open("datalog1.csv", FILE_WRITE);
 // jika file tersedia, tulislah
 if (dataFile1)
 {
dataFile1.print(dataString1);
dataFile1.println(dataString2);
dataFile1.close();
 }
 // jika file tidak terbuka, pop up error
 else
 {
  Serial.println("error opening datalog1.csv");
 }
 Serial.print(dataString1);
 Serial.print(dataString2);
 Serial.println();
 Serial.println();
 Serial.println();
 // setel ulang nilai untuk loop berikutnya
 i = -1;
 sumNH3 = 0;
 sumh1 = 0;
 sumt1 = 0;
 sumAlt1 = 0;
}