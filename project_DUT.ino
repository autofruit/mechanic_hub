#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "img_source.h"  // chứa mảng ảnh menu_boost

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // Không cần reset riêng
#define BUTTON_UP_PIN     D5
#define BUTTON_DOWN_PIN   D6
#define BUTTON_SELECT_PIN D7

#define HOLD_THRESHOLD 500  // Thời gian giữ lâu để thoát 1 chức năng (ms)

bool selectPressed = false;
bool selectHoldProcessed = false;
unsigned long selectPressStartTime = 0;
int menu_state = 0;
//----mode0_servo-phase---
bool mode0 = false;
bool staticDrawn0 = false;
int lastServo1 = 0;
int lastServo2 = 0;

bool mode1 = false;
bool mode2 = false;
bool mode3 = false;
bool mode_total = false;

int ser1;
int ser2;
int mode_ser = 0;

void set_servo(int ser_index){
  switch(ser_index){
    case 1:
    break;
    case 2:
    break;
    default:
    break;
  }
}

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void display_servo_phase(int currentServo1,int currentServo2){
  if (!staticDrawn0) {
    display.clearDisplay();
    display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    display.setTextColor(BLACK, WHITE);
    display.setTextSize(2);

    display.setCursor(5, 5);
    display.print("Servo1:");
    display.setCursor(5, 25);
    display.print("Servo2:");
    display.setCursor(90, 5);
    display.print(lastServo1);
    display.setCursor(90, 25);
    display.print(lastServo2);

    display.setTextSize(1);
    display.setCursor(5, 55);
    display.print("hold S to escape");

    display.display();
    staticDrawn0 = true;  // Đánh dấu đã vẽ chữ
  }

  // Chỉ cập nhật nếu giá trị thay đổi
  if (currentServo1 != lastServo1) {
    display.fillRect(90, 5, 30, 16, WHITE);    // Xóa vùng cũ
    display.setTextColor(BLACK, WHITE);
    display.setTextSize(2);
    display.setCursor(90, 5);
    display.print(currentServo1);
    display.display();
    lastServo1 = currentServo1;
  }
  if (currentServo2 != lastServo2) {
    display.fillRect(90, 25, 30, 16, WHITE);   // Xóa vùng cũ
    display.setTextColor(BLACK, WHITE);
    display.setTextSize(2);
    display.setCursor(90, 25);
    display.print(currentServo2);
    display.display();
    lastServo2 = currentServo2;
  }
  
}

void set_menu_state_on(int menu_index){
  switch(menu_index){
    case 0:
    mode0 = true;
    break;
    case 1:
    mode1 = true;
    break;
    case 2:
    mode2 = true;
    break;
    case 3:
    mode3 = true;
    break;
    default:
    mode0 = false;
    mode1 = false;
    mode2 = false;
    mode3 = false;
    break;
  }
}


void setup() {
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.begin(115200);
  delay(2000);
  Serial.println(F("Starting!"));

  // Khởi tạo giao tiếp I2C nếu dùng D1/D2 trên ESP8266 còn Arduino thì bỏ dòng này đi rồi SDA-A4,SCK/SCL vào A5 là nó mặc định.
  Wire.begin(D1, D2);  // SDA, SCL

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
  Serial.println(F("Initialized!"));

  display.clearDisplay();
  display.drawBitmap(0, 0, menu_boost, 128, 64, WHITE);
  display.display();
  delay(5000);
  display.clearDisplay();
  display.display();
}

void text_draw(String text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(BLACK, WHITE);
  display.fillRect(0, 0, 128, 64, WHITE);  // Nền trắng

  // Căn giữa văn bản
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (128 - w) / 2;
  int y = (64 - h) / 2;

  display.setCursor(x, y);
  display.println(text);
  display.display();
}

void loop() {
  bool mode_total = mode1 || mode2 || mode3|| mode0; 
  // --- Xử lý nút UP ---
  if (digitalRead(BUTTON_UP_PIN) == LOW) {
    Serial.println("UP");
    if(!mode_total){
      menu_state += 1;
    }else{
      if(mode_ser == 0){
        ser1 += (ser1<180)? 1 : 0;
      }else{
        ser2 += (ser2<180)? 1 : 0;
      }
    }
    delay(150); // debounce
  }

  // --- Xử lý nút DOWN ---
  if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
    Serial.println("DOWN");
    if(!mode_total){
      menu_state -= 1;
    }else{
      if(mode_ser == 0){
        ser1 -= (ser1>0)? 1 : 0;
      }else{
        ser2 -= (ser2>0)? 1 : 0;
      }
    }
    delay(150); // debounce
  }
  if(menu_state == 4){
    menu_state = 0;
  }
  if(menu_state == -1){
    menu_state = 3;
  }
  Serial.printf("ser1 la: %d\n",ser1);
  Serial.printf("ser2 la: %d\n",ser2);
  Serial.println(menu_state);

  // --- Xử lý nút SELECT (có phân biệt NHẤN NHANH / GIỮ LÂU) ---
  bool selectCurrentState = digitalRead(BUTTON_SELECT_PIN) == LOW;

  if (selectCurrentState && !selectPressed) {
    selectPressStartTime = millis();
    selectPressed = true;
    selectHoldProcessed = false;
  }

  if (selectCurrentState && selectPressed && !selectHoldProcessed) {
    if (millis() - selectPressStartTime > HOLD_THRESHOLD) {
      Serial.println("HOLD");
      set_menu_state_on(4);// so 4 co nghia la huy dieu kien
      selectHoldProcessed = true;
    }
  }

  if (!selectCurrentState && selectPressed) {
    if (!selectHoldProcessed) {
      Serial.println("PRESS");
      if(!mode_total){
        set_menu_state_on(menu_state);
      }else{
        mode_ser +=1;
      }
    }
    selectPressed = false;
  }
  if (mode_ser == 2){
    mode_ser = 0;
  }
  Serial.printf("dang o ser: %d\n",mode_ser);

  if(mode_total){//da chon 1 chuc nang
    switch(menu_state){
    case 0:
    display_servo_phase(ser1,ser2);
    break;
    case 1:
    text_draw("Coming Soon!");
    break;
    case 2:
    text_draw("Coming Soon!");
    break;
    case 3:
    text_draw("Coming Soon!");
    break;
    default:
    break;
  }
  }else{//khong chon gi ca
    switch(menu_state){
    case 0:
    display.clearDisplay();
    display.drawBitmap(0, 0,servo_phase, 128, 64, WHITE);
    display.display();
    staticDrawn0 = false;
    break;
    case 1:
    display.clearDisplay();
    display.drawBitmap(0, 0,F2_phase, 128, 64, WHITE);
    display.display();
    break;
    case 2:
    display.clearDisplay();
    display.drawBitmap(0, 0,F3_phase, 128, 64, WHITE);
    display.display();
    break;
    case 3:
    display.clearDisplay();
    display.drawBitmap(0, 0,wifi, 128, 64, WHITE);
    display.display();
    break;
    default:
    break;
    }
  }
  delay(10);
}

