#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define MQ2_PIN A0
#define FLAME_PIN A1

#define BUZZER_PIN 4
#define RELAY1_PIN 5   // Quạt – Kích mức cao
#define RELAY2_PIN 6   // Bơm – Kích mức cao

#define BUTTON_FAN 7
#define BUTTON_PUMP 8

#define THRESHOLD 600
#define PHONE_NUMBER "0383674008"
#define DETECT_DURATION 5000  // 5 giây

SoftwareSerial sim(2, 3); // RX, TX
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Biến trạng thái
bool called = false;
bool fanManual = false;
bool pumpManual = false;

bool lastFanBtn = HIGH;
bool lastPumpBtn = HIGH;

unsigned long gasStartTime = 0;
unsigned long flameStartTime = 0;

void setup() {
  Serial.begin(9600);
  sim.begin(115200);
  lcd.init();
  lcd.backlight();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(BUTTON_FAN, INPUT_PULLUP);
  pinMode(BUTTON_PUMP, INPUT_PULLUP);

  // Tắt quạt và bơm ban đầu
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.setCursor(0, 0);
  lcd.print("HE THONG BAT DAU");
  delay(2000);
  lcd.clear();
}

void loop() {
  int gasValue = analogRead(MQ2_PIN);
  int flameValue = digitalRead(FLAME_PIN); // LOW = có lửa
  bool gasDetected = gasValue > THRESHOLD;
  bool flameDetected = flameValue == LOW;

  unsigned long now = millis();

  // Ghi nhận thời gian phát hiện
  if (gasDetected) {
    if (gasStartTime == 0) gasStartTime = now;
  } else {
    gasStartTime = 0;
  }

  if (flameDetected) {
    if (flameStartTime == 0) flameStartTime = now;
  } else {
    flameStartTime = 0;
  }

  bool gasConfirmed = (gasStartTime > 0) && (now - gasStartTime >= DETECT_DURATION);
  bool flameConfirmed = (flameStartTime > 0) && (now - flameStartTime >= DETECT_DURATION);

  // Hiển thị LCD
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print("GAS:");
  lcd.print(gasValue);

  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("LUA:");
  lcd.print(flameDetected ? "CO   " : "KHONG");

  // Còi
  digitalWrite(BUZZER_PIN, (gasConfirmed || flameConfirmed) ? HIGH : LOW);

  // Điều khiển Relay – kích mức cao
  digitalWrite(RELAY1_PIN, (gasConfirmed || fanManual) ? HIGH : LOW);
  digitalWrite(RELAY2_PIN, (flameConfirmed || pumpManual) ? HIGH : LOW);

  // Gửi cảnh báo chỉ 1 lần
  if ((gasConfirmed || flameConfirmed) && !called) {
    String message;
    if (gasConfirmed && flameConfirmed) message = "CANH BAO: CO GAS + LUA!";
    else if (gasConfirmed) message = "CANH BAO: PHAT HIEN GAS!";
    else message = "CANH BAO: PHAT HIEN LUA!";

    sendSMS(message);
    delay(500);
    callNumber(PHONE_NUMBER);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SMS+CALL DA GUI");

    called = true;
    delay(2000);
    lcd.clear();
  }

  // Nút nhấn bật/tắt quạt
  bool fanBtn = digitalRead(BUTTON_FAN);
  if (fanBtn == LOW && lastFanBtn == HIGH && !gasConfirmed) {
    fanManual = !fanManual;
  }
  lastFanBtn = fanBtn;

  // Nút nhấn bật/tắt bơm
  bool pumpBtn = digitalRead(BUTTON_PUMP);
  if (pumpBtn == LOW && lastPumpBtn == HIGH && !flameConfirmed) {
    pumpManual = !pumpManual;
  }
  lastPumpBtn = pumpBtn;

  delay(200);
}

void sendSMS(String message) {
  sim.println("AT+CMGF=1"); delay(200);
  sim.print("AT+CMGS=\""); sim.print(PHONE_NUMBER); sim.println("\"");
  delay(200);
  sim.print(message);
  sim.write(26); // Ctrl+Z
  delay(1000);
}

void callNumber(String number) {
  sim.print("ATD"); sim.print(number); sim.println(";");
}
