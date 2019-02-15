#define REPS 1000
void setup() {
  Serial.begin(9600);
  while (!Serial);
  delay(2000);
  MCLK->APBCMASK.reg |= MCLK_APBCMASK_TRNG;   // TRNG clock on
  TRNG->CTRLA.reg = TRNG_CTRLA_ENABLE;
}

void loop() {
  uint32_t r, us;
  us = micros();
  for (int i = 0; i < REPS; i++) {
    while ((TRNG->INTFLAG.reg & TRNG_INTFLAG_DATARDY) == 0) ; // 84 APB cycles
    r = TRNG->DATA.reg;
  }
  us = micros() - us;
  Serial.print(us); Serial.print(" us   mbs ");
  Serial.println(32 * REPS / us);
  Serial.println(r, HEX);
  delay(2000);
}
