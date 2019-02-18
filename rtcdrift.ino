// rtcdrift
//  check drift on RTC in counter mode with various clock sources
// mode0  is 32-bit counter
//  hostdrift -f 32768


#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

#define LED_PIN 13

void clkinit() {
  RTC->MODE0.CTRLA.reg &= ~RTC_MODE0_CTRLA_ENABLE;  // disable
  while (RTC->MODE0.SYNCBUSY.bit.ENABLE); // sync
  RTC->MODE0.CTRLA.reg |= RTC_MODE0_CTRLA_SWRST; // software reset
  while (RTC->MODE0.SYNCBUSY.bit.SWRST); // sync
  OSC32KCTRL->RTCCTRL.reg = 5;  // 5 32khz crystal  1 ulp oscillator
  RTC->MODE0.CTRLA.reg |=  RTC_MODE0_CTRLA_COUNTSYNC | RTC_MODE0_CTRLA_PRESCALER(1)
                           | RTC_MODE0_CTRLA_ENABLE; // enable
  while (RTC->MODE0.SYNCBUSY.bit.ENABLE); // sync
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  delay(1000);
  PRREG(MCLK->APBAMASK.reg);
  PRREG(OSC32KCTRL->XOSC32K.reg);
  PRREG(OSC32KCTRL->RTCCTRL.reg);
  clkinit();
  PRREG(OSC32KCTRL->RTCCTRL.reg);
  PRREG(RTC->MODE0.CTRLA.reg);
}

void display() {
  uint32_t t;
  static uint32_t prev = 0;

  t = RTC->MODE0.COUNT.reg;
  Serial.print(t); Serial.print(" ");
  Serial.println(t - prev);
  prev = t;
  delay(1000);
}

void logger() {
  static long cnt = 0;
  uint32_t t;

  while (Serial.available() < 4);  // wait for host request
  t = RTC->MODE0.COUNT.reg;
  Serial.read();
  Serial.read();
  Serial.read();
  Serial.read();
  Serial.write((uint8_t *)&t, 4);
  cnt++;
  digitalWrite(LED_PIN, cnt & 1);
}

void loop() {
  display();
  //logger();
}
