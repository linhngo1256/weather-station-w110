/*
 * Author: RedFox97
 * Đấu nối
 * |---------|---------|---------------|
 * |   LCD   | ESP8266 | Chân đặc biệt |
 * |---------|---------|---------------|
 * |   RST   |    D0   |     GPIO16    |
 * |   SCLK  |    D6   |     GPIO12    |
 * |   A0    |    D5   |     GPIO14    |
 * |   SID   |    D7   |     GPIO13    |
 * |---------|---------|---------------|
 */

//Khai báo thư viện
#include <ESP8266WiFi.h>              // Thư viện esp8266 version 2.5.2
#include <time.h>
#include <Ticker.h>
#include <ESP8266HTTPClient.h>        // Thư viện HTTP Client version 0.4.0
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>              // Thư viện WiFi Manager version 0.14
#include <ArduinoJson.h>              // Thư viện decode Json version 6.13.0
#include "ST7565_homephone_esp8266.h"
#include "Img.h"
#include "data.h"
#include "hienthi.h"

// Khai báo chân nút bấm, còi
#define Select  5     //D1--GPIO5---------Chân số 1
#define Up      4     //D2--GPIO4---------Chân số 2
#define Down    0     //D3--GPIO0---------Chân số 3
#define Back    2     //D4--GPIO2---------Chân số 4
#define Buzz    15    //D8--GPIO15

// Tọa độ có thể tùy vào vị trí bạn ở
const String cityName   = "Ha Noi";
const String latitude   = "21.0651854";                         // Kinh độ
const String longitude  = "105.7185722";                        // Vĩ độ
const String key        = "79eff4254e1ebba42090b2f6445a5b4f";   // auth key weatherbit.io
int     lanChayHieuUng, viTriHieuUng, thoiGianChayHieuUng;

Ticker      flip;
WiFiManager wifiManager;

void setup() {
  Serial.begin(115200);                                       // Khởi tạo Serial với baudrate 115200
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");   // Cài đặt thời gian
  pinMode(Select,INPUT_PULLUP);
  pinMode(Up,INPUT_PULLUP);
  pinMode(Down,INPUT_PULLUP);
  pinMode(Back,INPUT_PULLUP);
  pinMode(Buzz,OUTPUT);
  lcd.ON();                                                   // Khởi tạo LCD
  lcd.SET(25,0,0,0,4);
  lcd.display();
  lcd.clear();                                                // Hiển thị
  lcd.Corner(0,0,127,63,5,BLACK);                             // Vẽ bo ngoài
  canGiua(64,12,"Dang cai dat...");                           // Thông báo đang cài đặt WiFi
  lcd.display();
  delay(500);
  wifiManager.autoConnect("Weather Station","12345678");      // Chờ kết nối WiFi
  canGiua(64,22,"Da ket noi WiFi");                           // Thông báo kết nối thành công WiFi
  lcd.display();
  delay(500);
  while(nam.toInt()<2000) {                                   // Chờ cập nhật thời gian xong
    capNhatThoiGian();                                        // Cập nhật thời gian
    delay(50);
  }
  duBaoHienTai(latitude, longitude, key);
  duBaoThoiTiet(latitude, longitude, key);
  canGiua(64,32,"Da cap nhat");                     // Thông báo cập nhật xong thời gian
  canGiua(64,42,"thoi gian");                     // Thông báo cập nhật xong thời gian
  lcd.display();
  delay(2000);
  flip.attach(1,ngat);                                        // Khởi tạo ngắt 1s
}

void loop() {
  lcd.clear();
  lcd.Corner(0,0,127,63,5,BLACK);
  lcd.DrawLine(0,10,128,10,BLACK);
  canGiua(64,1,cityName);                                      // Hiển thị tên Thành Phố
  Signal();
  lcd.DrawLine(0,34,128,34,BLACK);                            // Đường kẻ giữa thời gian và thông tin thời tiết
  canGiua(32,13,ngayTrongTuan);                                          // Hiển thị thời gian
  canGiua(32,24,ngay+"/"+thang+"/"+String(nam));
  hienThiGio();
  lcd.DrawLine(64 - viTriHieuUng, 34, 64 - viTriHieuUng, 64, BLACK);
  lcd.DrawLine(128 - viTriHieuUng, 34, 128 - viTriHieuUng, 64, BLACK);
  hienThiThoiTiet(18,36,ngay + "/" + thang,String(nhietDoHienTai),String(doAmHienTai));
  iconThoiTiet(38,38,thoiTietHienTai,1);
  hienThiThoiTiet(82,36,ngayMai + "/" + thangSau,String(nhietDoNgayMai),String(doAmNgayMai));
  iconThoiTiet(102,38,thoiTietNgayMai,2);
  lcd.display();
}

void ngat() {
  sec++;
  if(sec == 60) {
    minute++;
    sec = 0;
  }
  if(minute == 60) {
    hour++;
    minute = 0;
  }
  if(hour == 24) {                              // Sau 1 ngày cập nhật thời gian 1 lần
    hour = 0;
    capNhatThoiGian();
  }
  if(WiFi.status() != WL_CONNECTED)
    WiFi.reconnect();
}
