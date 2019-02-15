// samd51 AES CBC 128-bit key

#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)
#define NBYTES (1024)

void aes_init() {
  MCLK->APBCMASK.reg |= MCLK_APBCMASK_AES;
}

void aes_setkey(const uint8_t *key, size_t keySize) {
  memcpy((uint8_t *)&REG_AES_KEYWORD0, key, keySize);
}

void aes_cbc_encrypt(const uint8_t *plaintext, uint8_t *ciphertext, size_t size, const uint8_t iv[16])
{
  int i;

  memcpy((uint8_t *)&REG_AES_INTVECTV0, iv, 16);
  REG_AES_CTRLA = 0;
  REG_AES_CTRLA = AES_CTRLA_AESMODE_CBC | AES_CTRLA_CIPHER_ENC | AES_CTRLA_ENABLE;
  REG_AES_CTRLB |= AES_CTRLB_NEWMSG;
  uint32_t *wp = (uint32_t *) plaintext;   // need to do by word ?
  uint32_t *wc = (uint32_t *) ciphertext;
  // block 4-words  16B
  int word = 0;
  while (size > 0) {
    for (i = 0;  i < 4; i++) REG_AES_INDATA = wp[i + word];
    REG_AES_CTRLB |=  AES_CTRLB_START;
    while ((REG_AES_INTFLAG & AES_INTENCLR_ENCCMP) == 0);  // wait for done
    for (i = 0;  i < 4; i++) wc[i + word] = REG_AES_INDATA;
    size -= 16;
    word += 4;
  }
}

void aes_cbc_decrypt(const uint8_t *ciphertext, uint8_t *plaintext, size_t size, const uint8_t iv[16])
{
  int i;

  memcpy((uint8_t *)&REG_AES_INTVECTV0, iv, 16);
  REG_AES_CTRLA = 0;
  REG_AES_CTRLA = AES_CTRLA_AESMODE_CBC | AES_CTRLA_CIPHER_DEC | AES_CTRLA_ENABLE;
  REG_AES_CTRLB |= AES_CTRLB_NEWMSG ;
  uint32_t *wp = (uint32_t *) plaintext;   // need to do by word ?
  uint32_t *wc = (uint32_t *) ciphertext;
  // block 4-words  16B
  int word = 0;
  while (size > 0) {
    for (i = 0;  i < 4; i++) REG_AES_INDATA = wc[i + word];
    REG_AES_CTRLB |= AES_CTRLB_START;
    while ((REG_AES_INTFLAG & AES_INTENCLR_ENCCMP) == 0);  // wait for done
    for (i = 0;  i < 4; i++) wp[i + word] = REG_AES_INDATA;
    size -= 16;
    word += 4;
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  delay(1000);
  PRREG(MCLK->APBCMASK.reg);   //aes bit 9
  PRREG(REG_PAC_STATUSC);      // aes bit 9
  aes_init();

  PRREG(MCLK->APBCMASK.reg);
  PRREG(REG_AES_CTRLA);
  PRREG(REG_AES_CTRLB);
}

void loop() {
  static const uint8_t keyAes128[]  =
  { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
  };
  static const uint8_t ive[] =
  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
  };
  static const uint8_t plainAes128[] =
  { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
    0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
  };
  static const uint8_t cipherAes128[] =
  { 0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46,
    0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d
  };
  uint8_t  inmsg[NBYTES], cipherout[NBYTES], clearout[NBYTES];
  uint32_t t, i;

  for (i = 0; i < sizeof(inmsg); i++) inmsg[i] = i;
  memset(cipherout, 0, NBYTES);
  aes_setkey(keyAes128, 16);
  // verify
  aes_cbc_encrypt(plainAes128, cipherout, 16, ive);
  Serial.print("enc memcmp "); Serial.println(memcmp(cipherout, cipherAes128, 16));
  aes_cbc_decrypt(cipherout, clearout, 16, ive);
  Serial.print("dec memcmp "); Serial.println(memcmp(clearout, plainAes128, 16));

  memset(cipherout, 7, NBYTES);
  t = micros();
  aes_cbc_encrypt(inmsg, cipherout, sizeof(inmsg), ive);
  t = micros() - t;
  Serial.print(t); Serial.println(" us");
  aes_cbc_decrypt(cipherout, clearout, sizeof(inmsg), ive);
  Serial.print("memcmp "); Serial.println(memcmp(inmsg, clearout, sizeof(inmsg)));
  delay(3000);
}
