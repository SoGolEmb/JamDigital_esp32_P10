#include "angka_score.h"
#include <DMD32.h>
#include <RTClib.h>

// #define PIN_DMD_nOE 16
// #define PIN_DMD_A 19      // D19
// #define PIN_DMD_B 4       // D4
// #define PIN_DMD_CLK 18    // D18_SCK  is SPI Clock if SPI is used
// #define PIN_DMD_SCLK 2    // D02
// #define PIN_DMD_R_DATA 23 // D23_MOSI is SPI Master Out if SPI is used
// // Fire up the DMD library as dmd
#define brigthnessPin PIN_DMD_nOE
#define DISPLAYS_ACROSS 2
#define DISPLAYS_DOWN 1

DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);
RTC_DS3231 rtc;

// Timer setup
// create a hardware timer  of ESP32
#define HW_TIMER 3UL // 0 ~ 3
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
void IRAM_ATTR triggerScan() {
  // update dmd display
  portENTER_CRITICAL_ISR(&timerMux);
  dmd.scanDisplayBySPI();
  portEXIT_CRITICAL_ISR(&timerMux);
}

// setting PWM properties
const int freq = 10000;
const int ledChannel = 0;
const int resolution = 8;
uint8_t timA = 0, timB = 0;

// function
void drawDots();
void drawClock(uint8_t scoreA, uint8_t scoreB);
time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);

void setup() {
  Serial.begin(115200);

  // SETUP RTC MODULE
  if (!rtc.begin()) {
    Serial.println("RTC module is NOT found");
    Serial.flush();
    while (1)
      ;
  }

  // automatically sets the RTC to the date & time on PC this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // manually sets the RTC with an explicit date & time, for example to set
  // January 21, 2021 at 3am you would call:
  // rtc.adjust(DateTime(2021, 1, 21, 3, 0, 0));

  //  return the clock speed of the CPU
  uint8_t cpuClock = ESP.getCpuFreqMHz();
  // Use 1st timer of 4
  // devide cpu clock speed on its speed value by MHz to get 1us for each signal  of the timer
  timer = timerBegin(HW_TIMER, cpuClock, true);
  // Attach triggerScan function to our timer
  timerAttachInterrupt(timer, &triggerScan, true);
  // Set alarm to call triggerScan function
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer, 300, true);
  // Start an alarm
  timerAlarmEnable(timer);
  // clear/init the DMD pixels held in RAM
  dmd.clearScreen(true); // true is normal (all pixels off), false is negative (all pixels on)
  dmd.selectFont(angka_score);

  // brightness
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(brigthnessPin, ledChannel);
  ledcWrite(ledChannel, 10);
  // delay(5000);
}

uint8_t sec = 0;
bool enable_clear = false;
uint32_t clearDotTicks = 0;
void loop() {
  DateTime now = rtc.now();

  if (sec != now.second()) {
    sec = now.second();
    drawDots();
    digitalClockDisplay();
    enable_clear = true;
    clearDotTicks = millis();
  }
  if ((millis() - clearDotTicks >= 500) && enable_clear) {
    dmd.clearScreen(true);
    digitalClockDisplay();
    enable_clear = false;
  }

  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.println(now.second(), DEC);
}
void drawDots() {
  dmd.drawFilledBox(30, 3, 33, 6, 0);
  dmd.drawFilledBox(30, 9, 33, 12, 0);
}

void drawClock(uint8_t scoreA, uint8_t scoreB) {
  uint8_t _scoreA[2] = {0}, _scoreB[2] = {0};
  if (scoreA >= 100)
    scoreA = 0;
  _scoreA[0] = ((uint8_t)scoreA / 10) + 48;
  _scoreA[1] = ((uint8_t)scoreA % 10) + 48;
  dmd.drawChar(2, 0, _scoreA[0], 0);
  dmd.drawChar(14, 0, _scoreA[1], 0);
  if (scoreB >= 100)
    scoreB = 0;
  _scoreB[0] = ((uint8_t)scoreB / 10) + 48;
  _scoreB[1] = ((uint8_t)scoreB % 10) + 48;
  dmd.drawChar(38, 0, _scoreB[0], 0);
  dmd.drawChar(50, 0, _scoreB[1], 0);
}

void digitalClockDisplay() {
  // digital clock display of the time
  DateTime now = rtc.now();
  printDigits(now.minute());
  printDigits(now.second());
  drawClock(now.hour(), now.minute());
}

void printDigits(int digits) {
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}