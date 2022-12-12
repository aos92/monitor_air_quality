// hitung R0
void setup()
{
 Serial.begin(9600);
}

void loop()
{
 float sensor_volt;
 float RS_air; // resistansi sensor
 float R0; // R0 Tidak diketahui 
 float sensorValue = 0;
 for(int x = 0 ; x < 500 ; x++) // perulangan
 {
  sensorValue = sensorValue + analogRead(A3); // tambahkan nilai analog sensor 500 kali
 }
 sensorValue = sensorValue/500.0; // rata-rata bacaan
 sensor_volt = sensorValue * (5.0 / 1023.0); // transformasi ke tegangan
 RS_air = ((5.0 * 10.0) / sensor_volt) - 10.0; //hitung RS untuk 0 amonia amb
 R0 = RS_air / (3.6); //hitung R0

 // tampilkan nilai dalam serial
 Serial.print("R0 = ");
 Serial.println(R0);
 Serial.print("Rs_AIR = ");
 Serial.println(RS_air);
 delay(1000); // penundaan
}