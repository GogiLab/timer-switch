#include <Arduino.h>
#include <main.h>
#include <TM1638.h>
#include <TickTimer.h>
#include <Relay.h>
#include <MilliWork.h>

#define LED_RUN       D4      // GPIO 2  : S/W run blue LED
#define LED_BLINK_SEC 1
#define TM1638_STB    D0      // GPIO 16 : TM1638 STB
#define TM1638_CLK    D1      // GPIO 5  : TM1638 CLK
#define TM1638_DIO    D2      // GPIO 4  : TM1638 DIO
#define LED_ON        LOW
#define LED_OFF       HIGH
#define INTERVAL_MS 10
#define PUMP_ONOFF     D3      // GPIO 0  : external FAN on/off

uint32_t onTimeMin = 60;
uint32_t offTimeMin = 60;

MilliWork milliWork(INTERVAL_MS);
TickTimer ledTimer{LED_BLINK_SEC, Unit::SEC, INTERVAL_MS};
Relay ledBlinker{LED_RUN};
TickTimer onTimer(onTimeMin, Unit::MIN, INTERVAL_MS);
TickTimer offTimer(offTimeMin, Unit::MIN,INTERVAL_MS);
Relay pumpRelay(PUMP_ONOFF);
TM1638 tm1638{TM1638_STB, TM1638_CLK, TM1638_DIO};

void setup() {
  Serial.begin(115200);

  ledTimer.start();
  pumpRelay.on();
  onTimer.start();

  // uint8_t display_num[8] = {0};
  // uint8_t leds = 0xff;

  // tm1638.display(display_num, leds);
}

void loop() {
  if (!milliWork.check()) {
    return;
  }

  if (ledTimer.tick()) {
    ledBlinker.change();
    ledTimer.start();
  }

  bool change = keyHandle();

  if (onTimer.tick() || offTimer.tick() || change) {
    if (pumpRelay.isOn()) {
      Serial.println("Turn off pump");
      pumpRelay.off();
      onTimer.stop();
      offTimer.start();
    }
    else {
      Serial.println("Turn on pump");
      pumpRelay.on();
      offTimer.stop();
      onTimer.start();
    }
  }

  display();
}

void display() {
  uint8_t display_num[8] = {0};
  uint8_t leds = 0;
  uint32_t remain = 0;
  if (pumpRelay.isOn()) {
    remain = onTimer.remains();
    display_num[0] = remain / 60 % 60 / 10;
    display_num[1] = remain / 60 % 10;
    display_num[2] = remain % 60 / 10;
    display_num[3] = remain % 60 % 10;
    display_num[4] = offTimeMin / 60 / 10;
    display_num[5] = offTimeMin / 60 % 10;
    display_num[6] = offTimeMin % 60 / 10;
    display_num[7] = offTimeMin % 60 % 10;
  }
  else {
    remain = offTimer.remains();
    display_num[0] = onTimeMin / 60 / 10;
    display_num[1] = onTimeMin / 60 % 10;
    display_num[2] = onTimeMin % 60 / 10;
    display_num[3] = onTimeMin % 60 % 10;
    display_num[4] = remain / 60 % 60 / 10;
    display_num[5] = remain / 60 % 10;
    display_num[6] = remain % 60 / 10;
    display_num[7] = remain % 60 % 10;
  }
  uint32_t hours = remain / 3600;
  for (uint32_t i=0; i<hours; i++) {
    leds |= (0x01 << i);
  }
  uint8_t dots = 0x01000100;
  tm1638.display(display_num, dots, leds);
}

bool keyHandle()
{
  unsigned char key = tm1638.getKey();
  if (!key) {
    return false;
  }

  Serial.printf("keyed: %0x08x\n", key);
  if (pumpRelay.isOn()) {
    if (key < 0x10) {
      return true;
    }
    updateTimer(key);
    offTimer.set(offTimeMin);
  }
  else { // off
    if (key > 0x0f) {
      return true;
    }
    updateTimer(key);
    onTimer.set(onTimeMin);
  }
  return false;
}

void time2Array(uint8_t* tmArray)
{
  tmArray[0] = onTimeMin / 60 / 10;
  tmArray[1] = onTimeMin / 60 % 10;
  tmArray[2] = onTimeMin % 60 / 10;
  tmArray[3] = onTimeMin % 60 % 10;
  tmArray[4] = offTimeMin / 60 / 10;
  tmArray[5] = offTimeMin / 60 % 10;
  tmArray[6] = offTimeMin % 60 / 10;
  tmArray[7] = offTimeMin % 60 % 10;
}

void array2Time(uint8_t* tmArray)
{
  onTimeMin = (tmArray[0]*10 + tmArray[1])*60 + tmArray[2]*10 + tmArray[3];
  offTimeMin = (tmArray[4]*10 + tmArray[5])*60 + tmArray[6]*10 + tmArray[7];
}

void updateTimer(uint8_t key)
{
  uint8_t tmArray[8] = {0};
  time2Array(tmArray);
  for (int i=0; i<8; i++) {
    if (key & (0x01 << i)) {
      tmArray[i]++;
      if (i == 2 || i == 6) {
        tmArray[i] = tmArray[i] % 6;
      } 
      else {
        tmArray[i] = tmArray[i] % 10;
      }
    }
  }
  array2Time(tmArray);
}