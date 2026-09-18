//Khai báo thư viện
#include <ESP8266WiFi.h>              // Thư viện esp8266 version 2.5.2
#include <time.h>
#include <Ticker.h>
#include <ESP8266HTTPClient.h>        // Thư viện HTTP Client version 0.4.0
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>              // Thư viện WiFi Manager version 0.14
#include <ArduinoOTA.h>
#include <EEPROM.h>               // Cập nhật firmware OTA qua WiFi
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

// ==================== CHỌN THÀNH PHỐ ====================
// Thành phố được lưu vào EEPROM, khởi động lại vẫn giữ lựa chọn.
struct ThanhPho {
  const char* ten;
  const char* lat;
  const char* lon;
};

const ThanhPho danhSachThanhPho[] = {
  {"TRA VINH",   "9.9347",  "106.3452"},
  {"VINH LONG",  "10.2530", "105.9722"},
  {"CAN THO",    "10.0452", "105.7469"},
  {"HCM CITY",   "10.8231", "106.6297"},
  {"DA NANG",    "16.0544", "108.2022"},
  {"HUE",        "16.4637", "107.5909"},
  {"HA NOI",     "21.0278", "105.8342"},
  {"NHA TRANG",  "12.2388", "109.1967"}
};

const int SO_THANH_PHO = sizeof(danhSachThanhPho) / sizeof(danhSachThanhPho[0]);
const uint8_t EEPROM_CITY_ADDR = 0;

int cityIndex = 0;
int cityMenuIndex = 0;

String cityName = "TRA VINH";
String latitude = "9.9347";
String longitude = "106.3452";

const String key = "711a2fd0ea834d9a9216db69744c4d9e";   // auth key weatherbit.io
int     lanChayHieuUng, viTriHieuUng, thoiGianChayHieuUng;

Ticker      flip;
WiFiManager wifiManager;

// ==================== OTA ====================
bool otaDaKhoiTao = false;
const char* OTA_HOSTNAME = "WeatherStation";
const char* OTA_PASSWORD = "12345678";

// Trạng thái phản hồi từ Web WiFiManager
volatile bool webDaSaveWiFi = false;
volatile bool webDangXuLy = false;


// ==================== MENU + PHÍM BẤM ====================
enum MenuState {
  MAIN_SCREEN,
  MENU_SCREEN,
  CITY_SCREEN,
  ABOUT_SCREEN,
  OTA_SCREEN
};

MenuState menuState = MAIN_SCREEN;
int menuIndex = 0;

const char* menuItems[] = {
  "Doi WiFi",
  "Doi thanh pho",
  "OTA Update",
  "About"
};

const int MENU_COUNT = 4;

bool docPhim(int pin) {
  static unsigned long lastPress[4] = {0, 0, 0, 0};
  int index = 0;

  if (pin == Select) index = 0;
  else if (pin == Up) index = 1;
  else if (pin == Down) index = 2;
  else if (pin == Back) index = 3;

  if (digitalRead(pin) == LOW && millis() - lastPress[index] > 180) {
    lastPress[index] = millis();

    // Chờ nhả phím để tránh lặp khi giữ nút
    while (digitalRead(pin) == LOW) {
      yield();
    }
    return true;
  }

  return false;
}


// Khai báo trước để callback có thể gọi hàm hiển thị trạng thái.
void hienThiTrangThaiWiFi(const String &dong1,
                          const String &dong2 = "",
                          const String &dong3 = "");

// =========================================================
// CALLBACK WEB WiFiManager
// Xử lý phản hồi khi điện thoại:
// 1. vào AP cấu hình
// 2. mở WebServer
// 3. bấm Save và WiFi mới kết nối thành công
// =========================================================

void callbackVaoAP(WiFiManager *wm) {
  webDangXuLy = true;
  webDaSaveWiFi = false;

  Serial.println();
  Serial.println("=== WEB WIFI CONFIG ===");
  Serial.print("AP: ");
  Serial.println(wm->getConfigPortalSSID());
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  hienThiTrangThaiWiFi(
    "Weather Station",
    "192.168.4.1",
    "Dang mo Web..."
  );
}

void callbackWebServer() {
  Serial.println("WebServer WiFiManager: READY");
}

void callbackSaveWiFi() {
  webDaSaveWiFi = true;
  Serial.println("WEB: SAVE WIFI OK");
}

void hienThiTrangThaiWiFi(const String &dong1, const String &dong2, const String &dong3) {
  lcd.clear();
  lcd.Corner(0, 0, 127, 63, 5, BLACK);
  canGiua(64, 2, "DOI WiFi");
  lcd.DrawLine(0, 10, 128, 10, BLACK);

  canGiua(64, 20, dong1);
  if (dong2.length() > 0) canGiua(64, 34, dong2);
  if (dong3.length() > 0) canGiua(64, 48, dong3);

  lcd.display();
}

void hienThiDangQuet(int soLan) {
  static const char *dots[] = {"", ".", "..", "..."};

  hienThiTrangThaiWiFi(
    "Dang quet WiFi",
    String("Vui long cho") + dots[soLan % 4],
    "Khong tat nguon"
  );
}

void hienThiAPWiFi() {
  lcd.clear();
  lcd.Corner(0, 0, 127, 63, 5, BLACK);
  canGiua(64, 2, "DOI WiFi");
  lcd.DrawLine(0, 10, 128, 10, BLACK);

  canGiua(64, 20, "WiFi: Weather Station");
  canGiua(64, 34, "MK: 12345678");
  canGiua(64, 48, "192.168.4.1");
  lcd.display();
}

void doiWiFi() {
  // =========================================================
  // ĐỔI WiFi - LUỒNG ỔN ĐỊNH
  //
  // Weather Station
  //        ↓
  // Kết nối điện thoại
  //        ↓
  // WiFiManager tự mở captive portal
  //        ↓
  // Configure WiFi
  //        ↓
  // Chọn WiFi mới + nhập mật khẩu
  //        ↓
  // Save
  //        ↓
  // WiFiManager lưu SSID/password vào flash
  //        ↓
  // ESP tự kết nối lại WiFi mới
  // =========================================================

  webDaSaveWiFi = false;
  webDangXuLy = false;

  hienThiTrangThaiWiFi(
    "Dang khoi dong",
    "Weather Station",
    "Xin cho..."
  );
  delay(800);

  // Không gọi WiFi.softAP() thủ công.
  // Không gọi WiFi.reconnect() trong lúc portal đang chạy.
  // WiFiManager sẽ tự quản lý AP, DNS, HTTP server và STA.
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  delay(500);

  // Cho phép hiện tất cả mạng WiFi, kể cả tín hiệu yếu.
  wifiManager.setMinimumSignalQuality(0);

  // Portal tồn tại tối đa 5 phút.
  wifiManager.setConfigPortalTimeout(300);

  // Bật captive portal để điện thoại có thể tự hiện trang
  // đăng nhập/cấu hình sau khi kết nối Weather Station.
  wifiManager.setCaptivePortalEnable(true);

  // Chỉ hiển thị các mục cần thiết.
  const char* menu[] = {
    "wifi",
    "info",
    "exit"
  };
  wifiManager.setMenu(menu, 3);

  hienThiAPWiFi();
  delay(1000);

  // KHÔNG resetSettings() ở đây.
  // Giữ WiFi hiện tại cho tới khi người dùng chọn WiFi mới và bấm Save.
  // WiFiManager sẽ tự lưu thông tin mới vào flash sau khi kết nối thành công.

  hienThiTrangThaiWiFi(
    "Weather Station",
    "192.168.4.1",
    "Cho chon WiFi..."
  );
  delay(700);

  // Nhận phản hồi từ từng giai đoạn của trang web.
  wifiManager.setAPCallback(callbackVaoAP);
  wifiManager.setWebServerCallback(callbackWebServer);
  wifiManager.setSaveConfigCallback(callbackSaveWiFi);

  // WiFiManager tự tạo:
  // SSID: Weather Station
  // Password: 12345678
  // IP: 192.168.4.1
  //
  // Điện thoại -> Weather Station -> 192.168.4.1
  // -> Configure WiFi -> chọn mạng -> nhập mật khẩu -> Save.
  bool wifiOK = wifiManager.startConfigPortal(
    "Weather Station",
    "12345678"
  );

  // startConfigPortal() trả về sau khi Save thành công,
  // người dùng bấm Exit hoặc hết timeout.
  if (webDaSaveWiFi) {
    hienThiTrangThaiWiFi(
      "Da nhan Save",
      "Dang ket noi WiFi",
      "Xin cho..."
    );
    delay(500);
  }

  // WiFiManager thường đã tự kết nối sau Save.
  // Chờ thêm tối đa 20 giây để xử lý phản hồi cuối cùng.
  unsigned long batDau = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - batDau < 20000) {
    hienThiTrangThaiWiFi(
      "Dang ket noi",
      "WiFi moi...",
      "Xin cho..."
    );
    delay(250);
    yield();
  }

  if (wifiOK && WiFi.status() == WL_CONNECTED) {
    String wifiMoi = WiFi.SSID();

    Serial.println("=== WIFI SAVE RESPONSE ===");
    Serial.print("SSID: ");
    Serial.println(wifiMoi);
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    hienThiTrangThaiWiFi(
      "Da luu WiFi",
      wifiMoi,
      "Da ket noi"
    );
    delay(1500);

    otaDaKhoiTao = false;
    khoiTaoOTA();

    webDangXuLy = false;
    webDaSaveWiFi = false;
    menuState = MAIN_SCREEN;
    return;
  }

  // Exit hoặc timeout hoặc mật khẩu sai.
  Serial.println("WEB: EXIT / TIMEOUT / CONNECT FAIL");

  hienThiTrangThaiWiFi(
    "Chua ket noi",
    "Web da dong",
    "Thu lai trong MENU"
  );
  delay(1800);

  webDangXuLy = false;
  webDaSaveWiFi = false;
  menuState = MAIN_SCREEN;
}

void hienThiAbout() {
  lcd.clear();
  lcd.Corner(0, 0, 127, 63, 5, BLACK);

  canGiua(64, 2, "ABOUT");
  lcd.DrawLine(0, 10, 128, 10, BLACK);

  canGiua(64, 18, "WEATHER STATION");
  canGiua(64, 29, "linh199x");
  canGiua(64, 40, "OTA: WeatherStation");
  canGiua(64, 53, "BACK: Quay lai");

  lcd.display();
}


void hienThiOTA() {
  lcd.clear();
  lcd.Corner(0, 0, 127, 63, 5, BLACK);

  canGiua(64, 2, "OTA UPDATE");
  lcd.DrawLine(0, 10, 128, 10, BLACK);

  if (WiFi.status() == WL_CONNECTED) {
    canGiua(64, 20, "San sang cap nhat");
    canGiua(64, 34, WiFi.localIP().toString());
    canGiua(64, 48, "BACK: Quay lai");
  } else {
    canGiua(64, 20, "Chua co WiFi");
    canGiua(64, 34, "Khong the OTA");
    canGiua(64, 48, "BACK: Quay lai");
  }

  lcd.display();
}

void khoiTaoOTA() {
  if (otaDaKhoiTao || WiFi.status() != WL_CONNECTED) {
    return;
  }

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    // Không gọi hàm WiFi trong callback.
    Serial.println("OTA START");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA END");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA: %u%%\r", (progress * 100U) / total);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA ERROR[%u]\n", error);
  });

  ArduinoOTA.begin();
  otaDaKhoiTao = true;

  Serial.println();
  Serial.println("===== OTA READY =====");
  Serial.print("Hostname: ");
  Serial.println(OTA_HOSTNAME);
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// =========================================================
// CITY EEPROM + WEATHER
// =========================================================

void luuThanhPho() {
  EEPROM.write(EEPROM_CITY_ADDR, (uint8_t)cityIndex);
  EEPROM.commit();

  Serial.print("CITY SAVED: ");
  Serial.println(cityName);
}

void napThanhPho() {
  int saved = EEPROM.read(EEPROM_CITY_ADDR);

  if (saved < 0 || saved >= SO_THANH_PHO) {
    saved = 0;
  }

  cityIndex = saved;
  cityMenuIndex = cityIndex;

  cityName = danhSachThanhPho[cityIndex].ten;
  latitude = danhSachThanhPho[cityIndex].lat;
  longitude = danhSachThanhPho[cityIndex].lon;

  Serial.print("CITY LOADED: ");
  Serial.println(cityName);
}

void capNhatThanhPho() {
  cityName = danhSachThanhPho[cityIndex].ten;
  latitude = danhSachThanhPho[cityIndex].lat;
  longitude = danhSachThanhPho[cityIndex].lon;

  luuThanhPho();

  if (WiFi.status() == WL_CONNECTED) {
    hienThiTrangThaiWiFi(
      "THANH PHO",
      cityName,
      "Dang cap nhat..."
    );

    duBaoHienTai(latitude, longitude, key);
    duBaoThoiTiet(latitude, longitude, key);

    hienThiTrangThaiWiFi(
      "Da doi thanh pho",
      cityName,
      "Da cap nhat"
    );
    delay(1200);
  }
}

void hienThiChonThanhPho() {
  lcd.clear();
  lcd.Corner(0, 0, 127, 63, 5, BLACK);

  canGiua(64, 2, "CHON THANH PHO");
  lcd.DrawLine(0, 10, 128, 10, BLACK);

  // Hiển thị 4 thành phố quanh vị trí đang chọn.
  int start = (cityMenuIndex / 4) * 4;

  for (int i = 0; i < 4; i++) {
    int idx = start + i;
    if (idx >= SO_THANH_PHO) break;

    int y = 21 + i * 12;

    if (idx == cityMenuIndex) {
      lcd.DrawLine(2, y - 8, 124, y - 8, BLACK);
      lcd.DrawLine(2, y + 1, 124, y + 1, BLACK);
      lcd.DrawLine(2, y - 8, 2, y + 1, BLACK);
      lcd.DrawLine(124, y - 8, 124, y + 1, BLACK);
      canGiua(64, y - 6, String("> ") + danhSachThanhPho[idx].ten);
    } else {
      canGiua(64, y - 6, String("  ") + danhSachThanhPho[idx].ten);
    }
  }

  lcd.display();
}

void doiThanhPho() {
  cityMenuIndex = cityIndex;

  menuState = CITY_SCREEN;

  hienThiChonThanhPho();
}

void xuLyPhim() {
  if (docPhim(Select)) {
    digitalWrite(Buzz, HIGH);
    delay(25);
    digitalWrite(Buzz, LOW);

    if (menuState == MAIN_SCREEN) {
      menuState = MENU_SCREEN;
      menuIndex = 0;
    } else if (menuState == MENU_SCREEN) {
      if (menuIndex == 0) {
        doiWiFi();
        otaDaKhoiTao = false;
        khoiTaoOTA();
      } else if (menuIndex == 1) {
        doiThanhPho();
      } else if (menuIndex == 2) {
        menuState = OTA_SCREEN;
      } else if (menuIndex == 3) {
        menuState = ABOUT_SCREEN;
      }
    } else if (menuState == CITY_SCREEN) {
      // SELECT = xác nhận thành phố
      cityIndex = cityMenuIndex;
      capNhatThanhPho();
      menuState = MAIN_SCREEN;
    }
    return;
  }

  if (docPhim(Up)) {
    digitalWrite(Buzz, HIGH);
    delay(25);
    digitalWrite(Buzz, LOW);

    if (menuState == MENU_SCREEN) {
      menuIndex--;
      if (menuIndex < 0) menuIndex = MENU_COUNT - 1;
    } else if (menuState == CITY_SCREEN) {
      cityMenuIndex--;
      if (cityMenuIndex < 0) cityMenuIndex = SO_THANH_PHO - 1;
    }
    return;
  }

  if (docPhim(Down)) {
    digitalWrite(Buzz, HIGH);
    delay(25);
    digitalWrite(Buzz, LOW);

    if (menuState == MENU_SCREEN) {
      menuIndex++;
      if (menuIndex >= MENU_COUNT) menuIndex = 0;
    } else if (menuState == CITY_SCREEN) {
      cityMenuIndex++;
      if (cityMenuIndex >= SO_THANH_PHO) cityMenuIndex = 0;
    }
    return;
  }

  if (docPhim(Back)) {
    digitalWrite(Buzz, HIGH);
    delay(25);
    digitalWrite(Buzz, LOW);

    if (menuState == ABOUT_SCREEN || menuState == OTA_SCREEN ||
        menuState == CITY_SCREEN) {
      menuState = MENU_SCREEN;
    } else if (menuState == MENU_SCREEN) {
      menuState = MAIN_SCREEN;
    }
    return;
  }
}

void hienThiMenu() {
  lcd.clear();
  lcd.Corner(0, 0, 127, 63, 5, BLACK);

  canGiua(64, 2, "MENU");
  lcd.DrawLine(0, 10, 128, 10, BLACK);

  for (int i = 0; i < MENU_COUNT; i++) {
    int y = 21 + i * 13;

    if (i == menuIndex) {
      // ST7565_homephone_esp8266 không có DrawBox, nên vẽ khung bằng 4 đường.
      lcd.DrawLine(2, y - 9, 124, y - 9, BLACK);
      lcd.DrawLine(2, y + 1, 124, y + 1, BLACK);
      lcd.DrawLine(2, y - 9, 2, y + 1, BLACK);
      lcd.DrawLine(124, y - 9, 124, y + 1, BLACK);
      canGiua(64, y - 7, String("> ") + menuItems[i]);
    } else {
      canGiua(64, y - 7, String("  ") + menuItems[i]);
    }
  }

  lcd.display();
}

void setup() {
  Serial.begin(115200);                                       // Khởi tạo Serial với baudrate 115200
  EEPROM.begin(16);
  napThanhPho();
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");   // Cài đặt thời gian
  pinMode(Select,INPUT_PULLUP);
  pinMode(Up,INPUT_PULLUP);
  pinMode(Down,INPUT_PULLUP);
  pinMode(Back,INPUT_PULLUP);
  pinMode(Buzz,OUTPUT);
  lcd.ON();                                                   // Khởi tạo LCD
  lcd.SET(20,0,0,0,4);
  lcd.display();
  lcd.clear();                                                // Hiển thị
  lcd.Corner(0,0,127,63,5,BLACK);                             // Vẽ bo ngoài
  canGiua(64,12,"Dang cai dat...");                           // Thông báo đang cài đặt WiFi
  lcd.display();
  delay(500);
  wifiManager.setCaptivePortalEnable(true);
  wifiManager.setAPClientCheck(false);
  wifiManager.setWebPortalClientCheck(true);
  wifiManager.setConfigPortalTimeout(300);
  wifiManager.setConnectTimeout(20);

  bool wifiConnected = wifiManager.autoConnect("Weather Station","12345678"); // Tự kết nối WiFi đã lưu

  if (wifiConnected && WiFi.status() == WL_CONNECTED) {
    canGiua(64,22,"Da ket noi WiFi");
    lcd.display();
    khoiTaoOTA();
  } else {
    canGiua(64,22,"WiFi loi");
    lcd.display();
  }
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
  // OTA phải được gọi thường xuyên để nhận firmware từ Arduino IDE.
  if (WiFi.status() == WL_CONNECTED) {
    if (!otaDaKhoiTao) {
      khoiTaoOTA();
    }
    ArduinoOTA.handle();
  }

  // Kiểm tra WiFi ngoài ngắt Ticker để tránh lỗi và thao tác WiFi trong ISR.
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck >= 5000) {
    lastWiFiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
      otaDaKhoiTao = false;
    } else if (!otaDaKhoiTao) {
      khoiTaoOTA();
    }
  }

  // Đọc 4 phím:
  // SELECT = vào menu
  // UP     = lên
  // DOWN   = xuống
  // BACK   = trở về màn hình chính
  xuLyPhim();

  if (menuState == ABOUT_SCREEN) {
    hienThiAbout();
    delay(20);
    return;
  }

  if (menuState == OTA_SCREEN) {
    hienThiOTA();
    delay(100);
    return;
  }

  if (menuState == CITY_SCREEN) {
    hienThiChonThanhPho();
    delay(30);
    return;
  }

  if (menuState == MENU_SCREEN) {
    hienThiMenu();
    delay(20);
    return;
  }

  // ==================== MÀN HÌNH CHÍNH ====================
  lcd.clear();
  lcd.Corner(0,0,127,63,5,BLACK);
  lcd.DrawLine(0,10,128,10,BLACK);

  canGiua(64,1,cityName);
  Signal();

  lcd.DrawLine(0,34,128,34,BLACK);

  canGiua(32,13,ngayTrongTuan);
  canGiua(32,24,ngay+"/"+thang+"/"+String(nam));

  hienThiGio();

  lcd.DrawLine(64 - viTriHieuUng, 34,
               64 - viTriHieuUng, 64, BLACK);

  lcd.DrawLine(128 - viTriHieuUng, 34,
               128 - viTriHieuUng, 64, BLACK);

  hienThiThoiTiet(
    18,36,
    ngay + "/" + thang,
    String(nhietDoHienTai),
    String(doAmHienTai)
  );

  iconThoiTiet(38,38,thoiTietHienTai,1);

  hienThiThoiTiet(
    82,36,
    ngayMai + "/" + thangSau,
    String(nhietDoNgayMai),
    String(doAmNgayMai)
  );

  iconThoiTiet(102,38,thoiTietNgayMai,2);

  lcd.display();
  delay(20);
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
  // Không gọi WiFi.reconnect() trong ngắt Ticker.
  // Việc reconnect được xử lý trong loop() để tránh thao tác WiFi trong ISR.
}
