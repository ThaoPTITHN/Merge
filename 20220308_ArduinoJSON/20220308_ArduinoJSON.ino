#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <DHT.h>
#define DHTPIN 7
#define DHTTYPE DHT11
const byte RX = 2;
const byte TX = 3;
SoftwareSerial mySerial(RX,TX);
DHT dht(DHTPIN, DHTTYPE);
String ChuoiJSon = "";
int nhietdo = 0;
int doam = 0;
int StateTB1 = 0;
int StateTB2 = 0;
long cai1 = 0;
long cai2 = 0;
long last = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  mySerial.begin(115200);
  dht.begin();
  last = millis();
}
void cambien()
{
  nhietdo = dht.readTemperature();
  doam    = dht.readHumidity();
}
void DataJSON(String NhietDo, String DoAm, String TrangThaiTb1, String TrangThaiTb2, String Cai1, String Cai2)
{
  ChuoiJSon = "";
  ChuoiJSon = "{\"NhietDo\":\"" + String{NhietDo} + "\"," + 
               "\"Do Am\":\""  + String{DoAm} + "\", "+
               "\"TrangThaiTb1\":\"" + String{TrangThaiTb1} + "\","+
               "\"Trang Thai TB2\":\"" + String{TrangThaiTb2} + "\", "+
               "\"Cai Dat 1\":\"" + String{Cai1}+ "\", " + 
               "\"Cai dat 2\":\"" + String{Cai2}+ "\"}";
  //Dua du lieu vao arrduino JSOn 
  StaticJsonDocument<200> JSON; //taoj mang JSON 200byte
  //Duwa DL vao 
  deserializeJson(JSON, ChuoiJSon);
  JsonObject obj = JSON.as<JsonObject>();
  Serial.println("Send ESP");
  serializeJsonPretty(JSON, Serial);
  //Dua dl ra cong com ao
  serializeJsonPretty(JSON, mySerial);
  mySerial.flush();
  Serial.println(); 
}
void loop() {
  // put your main code here, to run repeatedly:
  if(millis() - last > 1000)
  {
    cambien();
    DataJSON(String(nhietdo), String(doam), String(StateTB1), String(StateTB2), String(cai1), String(cai2));
    last = millis();
  }
}
