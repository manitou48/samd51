// m4 SAMD51 chip temperature sensor on ADC
// Decimal to fraction conversion. (adapted from ASF sample).
//#define NVMCTRL_TEMP_LOG              (0x00800100)  // ref pg 59
#define NVMCTRL_TEMP_LOG NVMCTRL_TEMP_LOG_W0

static float convert_dec_to_frac(uint8_t val) {
  float float_val = (float)val;
  if (val < 10) {
    return (float_val / 10.0);
  } else if (val < 100) {
    return (float_val / 100.0);
  } else {
    return (float_val / 1000.0);
  }
}

static float calculate_temperature(uint16_t TP, uint16_t TC) {
  uint32_t TLI = (*(uint32_t *)FUSES_ROOM_TEMP_VAL_INT_ADDR & FUSES_ROOM_TEMP_VAL_INT_Msk) >> FUSES_ROOM_TEMP_VAL_INT_Pos;
  uint32_t TLD = (*(uint32_t *)FUSES_ROOM_TEMP_VAL_DEC_ADDR & FUSES_ROOM_TEMP_VAL_DEC_Msk) >> FUSES_ROOM_TEMP_VAL_DEC_Pos;
  float TL = TLI + convert_dec_to_frac(TLD);

  uint32_t THI = (*(uint32_t *)FUSES_HOT_TEMP_VAL_INT_ADDR & FUSES_HOT_TEMP_VAL_INT_Msk) >> FUSES_HOT_TEMP_VAL_INT_Pos;
  uint32_t THD = (*(uint32_t *)FUSES_HOT_TEMP_VAL_DEC_ADDR & FUSES_HOT_TEMP_VAL_DEC_Msk) >> FUSES_HOT_TEMP_VAL_DEC_Pos;
  float TH = THI + convert_dec_to_frac(THD);

  uint16_t VPL = (*(uint32_t *)FUSES_ROOM_ADC_VAL_PTAT_ADDR & FUSES_ROOM_ADC_VAL_PTAT_Msk) >> FUSES_ROOM_ADC_VAL_PTAT_Pos;
  uint16_t VPH = (*(uint32_t *)FUSES_HOT_ADC_VAL_PTAT_ADDR & FUSES_HOT_ADC_VAL_PTAT_Msk) >> FUSES_HOT_ADC_VAL_PTAT_Pos;

  uint16_t VCL = (*(uint32_t *)FUSES_ROOM_ADC_VAL_CTAT_ADDR & FUSES_ROOM_ADC_VAL_CTAT_Msk) >> FUSES_ROOM_ADC_VAL_CTAT_Pos;
  uint16_t VCH = (*(uint32_t *)FUSES_HOT_ADC_VAL_CTAT_ADDR & FUSES_HOT_ADC_VAL_CTAT_Msk) >> FUSES_HOT_ADC_VAL_CTAT_Pos;

  // From SAMD51 datasheet: section 45.6.3.1 (page 1327).
  return (TL * VPH * TC - VPL * TH * TC - TL * VCH * TP + TH * VCL * TP) / (VCL * TP - VCH * TP - VPL * TC + VPH * TC);
}

float get_tempc() {
  // enable and read 2 ADC temp sensors, 12-bit res
  volatile uint16_t ptat;
  volatile uint16_t ctat;
  SUPC->VREF.reg |= SUPC_VREF_TSEN | SUPC_VREF_ONDEMAND;
  ADC0->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_12BIT_Val;
  while (ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_CTRLB); //wait for sync
  while ( ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_INPUTCTRL ); //wait for sync
  ADC0->INPUTCTRL.bit.MUXPOS = ADC_INPUTCTRL_MUXPOS_PTAT;
  while ( ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_ENABLE ); //wait for sync
  ADC0->CTRLA.bit.ENABLE = 0x01;             // Enable ADC

  // Start conversion
  while ( ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_ENABLE ); //wait for sync

  ADC0->SWTRIG.bit.START = 1;

  // Clear the Data Ready flag
  ADC0->INTFLAG.reg = ADC_INTFLAG_RESRDY;

  // Start conversion again, since The first conversion after the reference is changed must not be used.
  ADC0->SWTRIG.bit.START = 1;

  while (ADC0->INTFLAG.bit.RESRDY == 0);   // Waiting for conversion to complete
  ptat = ADC0->RESULT.reg;

  while ( ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_INPUTCTRL ); //wait for sync
  ADC0->INPUTCTRL.bit.MUXPOS = ADC_INPUTCTRL_MUXPOS_CTAT;
  // Start conversion
  while ( ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_ENABLE ); //wait for sync

  ADC0->SWTRIG.bit.START = 1;

  // Clear the Data Ready flag
  ADC0->INTFLAG.reg = ADC_INTFLAG_RESRDY;

  // Start conversion again, since The first conversion after the reference is changed must not be used.
  ADC0->SWTRIG.bit.START = 1;

  while (ADC0->INTFLAG.bit.RESRDY == 0);   // Waiting for conversion to complete
  ctat = ADC0->RESULT.reg;


  while ( ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_ENABLE ); //wait for sync
  ADC0->CTRLA.bit.ENABLE = 0x00;             // Disable ADC
  while ( ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_ENABLE ); //wait for sync

  return calculate_temperature(ptat, ctat);
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  delay(1000);
}

void loop() {
  Serial.println(get_tempc());
  delay(2000);

}
