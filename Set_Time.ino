// Kode untuk menyetel waktu dari Tiny RTC dengan I2C multiplexer
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#define TCAADDR 0x70

const char *monthName[12] = {
"Jan", "Feb", "Mar", "Apr", 
"May", "Jun", "Jul", "Aug", 
"Sep", "Oct", "Nov", "Dec"
};

tmElements_t tm;

void tcaselect(uint8_t i)
{
 if (i > 7) return;
 Wire.beginTransmission(TCAADDR);
 Wire.write(1 << i);
 Wire.endTransmission();
}

void setup()
{
 tcaselect(7);
 bool parse=false;
 bool config=false;
 // dapatkan tanggal dan waktu compiler dijalankan
 if (getDate(__DATE__) && getTime(__TIME__)) 
 {
  parse = true;
  // dan konfigurasikan RTC dengan info ini
  if (RTC.write(tm)) 
  {
   config = true;
  }
 }
 Serial.begin(9600);
 while (!Serial) ; // tunggu Arduino Serial Monitor
 delay(200);

 if (parse && config)
 {
  Serial.print("DS1307 configured Time=");
  Serial.print(__TIME__);
  Serial.print(", Date=");
  Serial.println(__DATE__);
 }
 else if (parse)
 {
  Serial.println("DS1307 Communication Error :-{");
  Serial.println("Please check your circuitry");
 }
 else
 {
  Serial.print("Could not parse info from the compiler, Time=\"");
  Serial.print(__TIME__);
  Serial.print("\", Date=\"");
  Serial.print(__DATE__);
  Serial.println("\"");
 }
}

void loop(){}

bool getTime(const char *str)
{
 tcaselect(7);
 int Hour, Min, Sec;

 if (sscanf(str, "%d: %d: %d", &Hour, &Min, &Sec) != 3) 
 return false;

 tm.Hour   = Hour; 
 tm.Minute = Min;
 tm.Second = Sec;
 return true;
}

bool getDate(const char *str)
{
 tcaselect(7);
 char Month[12];
 int Day, Year;
 uint8_t monthIndex;

 if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) 
 return false;

 for (monthIndex = 0; monthIndex < 12; monthIndex++)
 {
  if (strcmp(Month, monthName[monthIndex]) == 0) break;
 }

 if (monthIndex >= 12) 
 return false;
 tm.Day   = Day;
 tm.Month = monthIndex + 1;
 tm.Year  = CalendarYrToTm(Year);
 return true;
}

