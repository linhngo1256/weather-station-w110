
#define DATA_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>


// =====================================================
// BIẾN THỜI GIAN
// =====================================================

String ngayTrongTuan, thang, ngay, gio, phut, giay, nam;

int hour = 0;
int minute = 0;
int sec = 0;


// =====================================================
// BIẾN THỜI TIẾT
// =====================================================

int nhietDoHienTai = 0;
int doAmHienTai = 0;
int thoiTietHienTai = 0;

int nhietDoNgayMai = 0;
int doAmNgayMai = 0;
int thoiTietNgayMai = 0;

int nhietDoNgayKia = 0;
int doAmNgayKia = 0;
int thoiTietNgayKia = 0;


// =====================================================
// NGÀY DỰ BÁO
// =====================================================

String ngayMai;
String thangSau;


// =====================================================
// HTTPS CLIENT
// =====================================================

WiFiClientSecure weatherClient;


// =====================================================
// CẬP NHẬT THỜI GIAN
// =====================================================

void capNhatThoiGian()
{
    time_t now = time(nullptr);

    if (now <= 0)
    {
        Serial.println("LOI: Khong co thoi gian!");
        return;
    }

    String data = ctime(&now);

    if (data.length() < 24)
    {
        Serial.println("LOI: Du lieu thoi gian khong hop le!");
        return;
    }

    // -------------------------------------------------
    // Tách dữ liệu
    // -------------------------------------------------

    ngayTrongTuan = data.substring(0, 3);

    String month = data.substring(4, 7);

    ngay = data.substring(8, 10);
    gio  = data.substring(11, 13);
    phut = data.substring(14, 16);
    giay = data.substring(17, 19);
    nam  = data.substring(20, 24);


    // -------------------------------------------------
    // Chuyển đổi
    // -------------------------------------------------

    ngayTrongTuan.toUpperCase();

    ngay.trim();

    if (ngay.toInt() < 10)
    {
        ngay = "0" + ngay;
    }


    // -------------------------------------------------
    // Chuyển tháng
    // -------------------------------------------------

    if (month == "Jan")
        thang = "01";
    else if (month == "Feb")
        thang = "02";
    else if (month == "Mar")
        thang = "03";
    else if (month == "Apr")
        thang = "04";
    else if (month == "May")
        thang = "05";
    else if (month == "Jun")
        thang = "06";
    else if (month == "Jul")
        thang = "07";
    else if (month == "Aug")
        thang = "08";
    else if (month == "Sep")
        thang = "09";
    else if (month == "Oct")
        thang = "10";
    else if (month == "Nov")
        thang = "11";
    else if (month == "Dec")
        thang = "12";


    // -------------------------------------------------
    // String -> int
    // -------------------------------------------------

    hour = gio.toInt();
    minute = phut.toInt();
    sec = giay.toInt();
}


// =====================================================
// THỜI TIẾT HIỆN TẠI
// =====================================================

void duBaoHienTai(String latitude,
                  String longitude,
                  String key)
{
    HTTPClient http;

    String url =
        "https://api.weatherbit.io/v2.0/current?lat=";

    url += latitude;
    url += "&lon=";
    url += longitude;
    url += "&key=";
    url += key;


    Serial.println();
    Serial.println("==============================");
    Serial.println("WEATHERBIT HIEN TAI");
    Serial.println("==============================");


    // -------------------------------------------------
    // HTTPS
    // -------------------------------------------------

    weatherClient.setInsecure();


    // -------------------------------------------------
    // ESP8266 Core 3.x
    // -------------------------------------------------

    if (!http.begin(weatherClient, url))
    {
        Serial.println("HTTP BEGIN FAILED");
        return;
    }


    http.setTimeout(10000);


    // -------------------------------------------------
    // GET
    // -------------------------------------------------

    int httpCode = http.GET();

    Serial.print("HTTP CODE: ");
    Serial.println(httpCode);


    // =================================================
    // THÀNH CÔNG
    // =================================================

    if (httpCode == HTTP_CODE_OK)
    {
        String data = http.getString();

        Serial.print("JSON SIZE: ");
        Serial.println(data.length());


        // -------------------------------------------------
        // JSON
        // -------------------------------------------------

        DynamicJsonDocument doc(4096);

        DeserializationError error =
            deserializeJson(doc, data);


        if (error)
        {
            Serial.print("JSON ERROR: ");
            Serial.println(error.c_str());

            http.end();
            return;
        }


        // -------------------------------------------------
        // Kiểm tra data
        // -------------------------------------------------

        if (!doc["data"].is<JsonArray>())
        {
            Serial.println("LOI: Khong co data[]");

            http.end();
            return;
        }


        JsonArray arr = doc["data"];


        if (arr.size() < 1)
        {
            Serial.println("LOI: data[] rong");

            http.end();
            return;
        }


        // -------------------------------------------------
        // Nhiệt độ
        // -------------------------------------------------

        nhietDoHienTai =
            doc["data"][0]["temp"].as<float>();


        // -------------------------------------------------
        // Độ ẩm
        // -------------------------------------------------

        doAmHienTai =
            doc["data"][0]["rh"].as<int>();


        // -------------------------------------------------
        // Weather code
        // -------------------------------------------------

        thoiTietHienTai =
            doc["data"][0]["weather"]["code"].as<int>();


        // -------------------------------------------------
        // SERIAL
        // -------------------------------------------------

        Serial.println();
        Serial.println("----- HIEN TAI -----");

        Serial.print("Nhiet do: ");
        Serial.print(nhietDoHienTai);
        Serial.println(" C");

        Serial.print("Do am: ");
        Serial.print(doAmHienTai);
        Serial.println(" %");

        Serial.print("Weather code: ");
        Serial.println(thoiTietHienTai);
    }
    else
    {
        Serial.print("HTTP ERROR: ");
        Serial.println(httpCode);

        if (httpCode > 0)
        {
            Serial.println(http.getString());
        }
        else
        {
            Serial.println(
                http.errorToString(httpCode)
            );
        }
    }


    http.end();
}


// =====================================================
// DỰ BÁO THỜI TIẾT
// NGÀY MAI + NGÀY KIA
// =====================================================

void duBaoThoiTiet(String latitude,
                   String longitude,
                   String key)
{
    HTTPClient http;


    // -------------------------------------------------
    // URL
    // -------------------------------------------------

    String url =
        "https://api.weatherbit.io/v2.0/forecast/daily?";

    url += "lat=";
    url += latitude;

    url += "&lon=";
    url += longitude;

    // 3 ngày:
    // data[0] = hôm nay
    // data[1] = ngày mai
    // data[2] = ngày kia

    url += "&days=3";

    url += "&key=";
    url += key;


    Serial.println();
    Serial.println("==============================");
    Serial.println("WEATHERBIT DU BAO");
    Serial.println("==============================");


    // -------------------------------------------------
    // HTTPS
    // -------------------------------------------------

    weatherClient.setInsecure();


    if (!http.begin(weatherClient, url))
    {
        Serial.println("FORECAST HTTP BEGIN FAILED");
        return;
    }


    http.setTimeout(10000);


    // -------------------------------------------------
    // GET
    // -------------------------------------------------

    int httpCode = http.GET();

    Serial.print("FORECAST HTTP CODE: ");
    Serial.println(httpCode);


    // =================================================
    // THÀNH CÔNG
    // =================================================

    if (httpCode == HTTP_CODE_OK)
    {
        String data = http.getString();

        Serial.print("FORECAST JSON SIZE: ");
        Serial.println(data.length());


        // -------------------------------------------------
        // JSON
        // -------------------------------------------------

        DynamicJsonDocument root(8192);

        DeserializationError error =
            deserializeJson(root, data);


        if (error)
        {
            Serial.print("FORECAST JSON ERROR: ");
            Serial.println(error.c_str());

            http.end();
            return;
        }


        // -------------------------------------------------
        // Kiểm tra data[]
        // -------------------------------------------------

        if (!root["data"].is<JsonArray>())
        {
            Serial.println(
                "LOI: Forecast khong co data[]"
            );

            http.end();
            return;
        }


        JsonArray arr = root["data"];


        if (arr.size() < 2)
        {
            Serial.println(
                "LOI: Khong du du lieu ngay mai"
            );

            http.end();
            return;
        }


        // =================================================
        // NGÀY MAI
        // =================================================

        nhietDoNgayMai =
            root["data"][1]["temp"].as<float>();

        doAmNgayMai =
            root["data"][1]["rh"].as<int>();

        thoiTietNgayMai =
            root["data"][1]["weather"]["code"].as<int>();


        String dateMai =
            root["data"][1]["datetime"].as<String>();


        if (dateMai.length() >= 10)
        {
            ngayMai =
                dateMai.substring(8, 10);

            thangSau =
                dateMai.substring(5, 7);
        }


        Serial.println();
        Serial.println("----- NGAY MAI -----");

        Serial.print("Ngay: ");
        Serial.print(ngayMai);
        Serial.print("/");
        Serial.println(thangSau);

        Serial.print("Nhiet do: ");
        Serial.print(nhietDoNgayMai);
        Serial.println(" C");

        Serial.print("Do am: ");
        Serial.print(doAmNgayMai);
        Serial.println(" %");

        Serial.print("Weather code: ");
        Serial.println(thoiTietNgayMai);


        // =================================================
        // NGÀY KIA
        // =================================================

        if (arr.size() >= 3)
        {
            nhietDoNgayKia =
                root["data"][2]["temp"].as<float>();

            doAmNgayKia =
                root["data"][2]["rh"].as<int>();

            thoiTietNgayKia =
                root["data"][2]["weather"]["code"].as<int>();


            Serial.println();
            Serial.println("----- NGAY KIA -----");

            Serial.print("Nhiet do: ");
            Serial.print(nhietDoNgayKia);
            Serial.println(" C");

            Serial.print("Do am: ");
            Serial.print(doAmNgayKia);
            Serial.println(" %");

            Serial.print("Weather code: ");
            Serial.println(thoiTietNgayKia);
        }
    }
    else
    {
        Serial.print("FORECAST HTTP ERROR: ");
        Serial.println(httpCode);

        if (httpCode > 0)
        {
            Serial.println(http.getString());
        }
        else
        {
            Serial.println(
                http.errorToString(httpCode)
            );
        }
    }


    http.end();
}

