// sha256 part of icm.h
//  user must do last block padding etc.
// region 0

#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

static IcmDescriptor dscr[4] __attribute__((aligned(64)));  // RADDR RCFG RCTRL RNEXT
static uint8_t hash[128] __attribute__((aligned(128)));
static uint8_t blocks[256 * 64]; // data

void prhash(unsigned char *h, int n) {
  int i;

  for (i = 0; i < n; i++) {
    Serial.print( h[i], HEX);
    Serial.print(" ");
    if (i % 4 == 3) Serial.print(" ");
  }
  Serial.println();
}

// need hash context to build up blocks and create final block
void sha_init() {

}

void sha_update() {

}

void sha_final() {

}

void setup() {
  uint32_t t;
  Serial.begin(9600);
  while (!Serial);
  delay(1000);
  PRREG(MCLK->AHBMASK.reg);   //icm bit 19
  PRREG(MCLK->APBCMASK.reg);  // icm bit 11
  PRREG(REG_PAC_STATUSC);    //icm bit 11
  MCLK->APBCMASK.reg |= MCLK_APBCMASK_ICM;
  PRREG(MCLK->APBCMASK.reg);

  PRREG(REG_ICM_CFG);
  PRREG(REG_ICM_CTRL);
  PRREG(REG_ICM_SR);

  dscr[0].RADDR.reg = (uint32_t)blocks;
  dscr[0].RCFG.reg = ICM_RCFG_ALGO(1) | ICM_RCFG_EOM_YES;  //RSA256
  dscr[0].RCTRL.reg = sizeof(blocks) / 64; // number of blocks
  dscr[0].RNEXT.reg = 0;
  REG_ICM_DSCR = (uint32_t) &dscr;
  REG_ICM_HASH = (uint32_t) hash;

  REG_ICM_CFG = 0;  // use RCFG
  REG_ICM_CTRL =  ICM_CTRL_ENABLE ;
  t = micros();
  while ( (REG_ICM_ISR & ICM_ISR_RHC(1)) == 0); // wait for done
  t = micros() - t;
  Serial.print(sizeof(blocks)); Serial.print(" bytes  ");
  Serial.print(t); Serial.println(" us");
  prhash(hash, 32);
  PRREG(REG_ICM_CFG);
  PRREG(REG_ICM_CTRL);
  PRREG(REG_ICM_SR);
  PRREG(dscr[0].RADDR.reg);
  PRREG(dscr[0].RCFG.reg);
  PRREG(dscr[0].RCTRL.reg);
  PRREG(dscr[0].RNEXT.reg);
}

void loop() {
}
