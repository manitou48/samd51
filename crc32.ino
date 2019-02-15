// crc32  for adafruit M4

#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

uint32_t buff[4 * 1024]; // word aligned 16KB

void setup() {
  Serial.begin(9600);
  while (!Serial);
  delay(2000);
  PRREG(REG_DSU_CTRL);
  PRREG(REG_DSU_STATUSA);
  PRREG(REG_PAC_STATUSB);
  REG_PAC_WRCTRL = 1 << 16 | 32 + 1; // write enable DSU
  PRREG(REG_PAC_STATUSB);
  for (int i = 0; i < sizeof(buff) / 4 ; i++) buff[i] = 1;
}

void loop() {
  uint32_t t = micros();
  REG_DSU_LENGTH = sizeof(buff); // length in  words << 2
  REG_DSU_ADDR = (uint32_t)buff;
  REG_DSU_DATA = 0xffffffff;
  REG_DSU_CTRL |= DSU_CTRL_CRC;
  while ((REG_DSU_STATUSA & DSU_STATUSA_DONE) == 0 );
  uint32_t val = REG_DSU_DATA ^ 0xffffffff;  // flip bits
  t = micros() - t;
  Serial.print(t); Serial.println(" us");
  Serial.println(val, HEX);
  PRREG(REG_DSU_STATUSA);
  delay(2000);
}
