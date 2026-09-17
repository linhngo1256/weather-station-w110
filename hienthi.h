int dBm, quality, percent;
ST7565 lcd(16, 12, 14, 13);       //RST, SCLK, A0, SID

/*---------------------------Hiển thị 1 chuỗi kí tự---------------------------*/
void hienThi(int x, int y,String chuoi) {
  lcd.Asc_String(x,y,(char*) chuoi.c_str(),BLACK);
}

/*---------------------------Căn giữa 1 chuỗi kí tự---------------------------*/
void canGiua(int x, int y, String a) {         // Căn giữa 1 chuỗi kí tự
  lcd.Asc_String(x+1-(a.length()*6)/2,y+1,(char*) a.c_str(),BLACK);
}

/*--------------------------Hiển thị số, font CASIO---------------------------*/
void hienThiSoCasio(int x,int y,int so,bool color) {      // Hiện thị 1 số theo font Casio
  if(so<10) {
    lcd.Number_Ulong(x,y,0,CASIO_NUMBER,color);
    lcd.Number_Ulong(x+13,y,so,CASIO_NUMBER,color);
  }
  else
    lcd.Number_Ulong(x,y,so,CASIO_NUMBER,color);
}

/*---------------------------Hiển thị thời gian------------------------------*/
void hienThiGio() {
  hienThiSoCasio(66,15,hour,BLACK);
  if(sec%2==0)
    lcd.Rect(95,19,2,9,BLACK);          // Dấu ':'
  else
    lcd.Rect(95,19,2,9,WHITE);
  lcd.Rect(95,21,2,5,WHITE);
  hienThiSoCasio(100,15,minute,BLACK);
}

void Signal() {                     // Hiển thị cường độ tín hiệu WiFi
  int percent;
  dBm = WiFi.RSSI();                              // Cường độ tín hiệu
  if(dBm<=-100 || WiFi.status() != WL_CONNECTED)  // Nếu cường độ < -100dBm thì không có tín hiệu WiFi
    quality=0;
  else                                            // Nếu cường độ >-50dBm thì cường độ tín hiệu tốt
    if(dBm>=-50)
      quality=100;
    else                                          // Cường độ tín hiệu tính theo công thức 2 * ( dBm + 100)
      quality = 2*(dBm+100);
  if(quality == 0)
    percent = 0;
  else
    if(quality<=20)
      percent = 1;
    else
      if(quality<=40)
        percent = 2;
      else
        if(quality<=60)
          percent = 3;
        else
          if(quality<=80)
            percent = 4;
          else
            percent = 5;
  for(int i=0;i<percent;i++) {                      // Hiển thị cột sóng
    lcd.Rect(4+2*i-2,8-i,1,i+1,BLACK);
  }
}

void iconThoiTiet(int x, int y,int code,int ngaybao) {      //https://www.weatherbit.io/api/codes   Hiển thị Icon thời tiết
  if(code==200 || code==201 || code==202)       // Mua kem sam set
    lcd.Bitmap(x,y,24,24,lighting_rain,BLACK);
  if(code==230 || code==231 || code==232 || code==233)   // Sam set
    lcd.Bitmap(x,y,24,24,lighting,BLACK);
  if(code==300 || code==301 || code==302 || code==500 || code==501 || code==502 || code==511 || code==520 || code==521 || code==522 || code==623)// May + mua
    lcd.Bitmap(x,y,24,24,rain,BLACK);
  if(ngaybao==1) {
    if(hour <18 && hour>5) {
      if(code==800)                                 // Clear Sky
        lcd.Bitmap(x,y,24,24,clear_sky,BLACK);
      if(code==801 || code==802 || code==803 || code==700 || code==711 || code==721 || code==731 || code==741 || code==751)       // Code 800 + may
        lcd.Bitmap(x,y,24,24,clear_cloudy,BLACK);
    }
    else {
      if(code==800)                                 // Clear Sky
        lcd.Bitmap(x,y,24,24,night,BLACK);
      if(code==801 || code==802 || code==803 || code==700 || code==711 || code==721 || code==731 || code==741 || code==751)       // Code 800 + may
        lcd.Bitmap(x,y,24,24,night_cloud,BLACK);
    }
  }
  else {
    if(code==800)                                 // Clear Sky
      lcd.Bitmap(x,y,24,24,clear_sky,BLACK);
    if(code==801 || code==802 || code==803 || code==700 || code==711 || code==721 || code==731 || code==741 || code==751)       // Code 800 + may
      lcd.Bitmap(x,y,24,24,clear_cloudy,BLACK);
  }
  if(code==611 || code==612 || code==804 )                                // may X2
    lcd.Bitmap(x,y,24,24,cloudy,BLACK);
}

void hienThiThoiTiet(int x, int y,String Day,String nhietdo, String doam) {     // Hiển thị form nhiệt độ, độ ẩm
  lcd.FillRect(x-15,y,Day.length()*6+1,9,BLACK);          // Ngày
  lcd.Asc_String(x-14,y+1,(char*) Day.c_str(),WHITE);
  if(nhietdo.toInt() < 10) {                             // Nhiệt độ
    hienThi(x-9,y+10,nhietdo);
    lcd.Bitmap(x-3,y+10,5,7,tdo,BLACK);
    lcd.Asc_Char(x+3,y+10,'C',BLACK);
  }
  else {
    hienThi(x-11,y+10,nhietdo);
    lcd.Bitmap(x+1,y+10,5,7,tdo,BLACK);
    lcd.Asc_Char(x+7,y+10,'C',BLACK);
  }
  canGiua(x,y+18,doam+"%");                                // Độ ẩm
}
