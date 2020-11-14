// m4 PPW capture period  pulse-width, free running timer @120mhz, DMA
// https://forum.arduino.cc/index.php?topic=673692.0
//  PPW capture on pin A4 with DMA  TCC1
//  test with pin 10 PWM to A4

#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

volatile uint32_t period;
volatile uint32_t pulseWidth;

typedef struct
{
  uint16_t btctrl;
  uint16_t btcnt;
  uint32_t srcaddr;
  uint32_t dstaddr;
  uint32_t descaddr;
} dmacdescriptor ;

volatile dmacdescriptor wrb[12] __attribute__ ((aligned (16)));               // Write-back DMAC descriptors
dmacdescriptor descriptor_section[12] __attribute__ ((aligned (16)));         // DMAC channel descriptors
dmacdescriptor descriptor __attribute__ ((aligned (16)));

void pwmWrite(int div, int duty) {
  //configure pin 10 for PWM PA20  TCC0 CH0
  GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GCLK_PCHCTRL_CHEN |        // Enable the TCC1 perhipheral channel
                                    GCLK_PCHCTRL_GEN_GCLK0;    // Connect 120MHz generic clock 0 to TCC0

  PORT->Group[PORTA].PINCFG[20].bit.PMUXEN = 1;
  PORT->Group[PORTA].PMUX[20 >> 1].reg |= PORT_PMUX_PMUXE(6);
  TCC0->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;            // Set-up TCC0 timer for Normal (single slope) PWM mode (NPWM)
  while (TCC0->SYNCBUSY.bit.WAVE);                   // Wait for synchronization

  TCC0->PER.reg = div - 1;          // 120mhz/(PER+1)  /11 1mhz
  while (TCC0->SYNCBUSY.bit.PER);         // Wait for synchronization

  TCC0->CC[0].reg = duty;                               //  duty-cycle
  while (TCC0->SYNCBUSY.bit.CC0);                    // Wait for synchronization

  TCC0->CTRLA.bit.ENABLE = 1;                        // Enable timer TCC0
  while (TCC0->SYNCBUSY.bit.ENABLE);                 // Wait for synchronization
}

void capture_init() {
  // setup PPW event capture of TCC1 clock with DMA, A4 interrupt PA04/EXTINT[4]

  // Enable DMAC
  DMAC->CTRL.bit.DMAENABLE = 0;                                     // Disable DMA to edit it
  DMAC->BASEADDR.reg = (uint32_t)descriptor_section;                // Set the descriptor section base
  DMAC->WRBADDR.reg = (uint32_t)wrb;                                // Set the write-back descriptor base adddress
  DMAC->CTRL.reg = DMAC_CTRL_LVLEN(0xF) | DMAC_CTRL_DMAENABLE;      // Enable all priority levels and enable DMA

  // Set up DMAC Channel 0 /////////////////////////////////////////////////////////////////////
  DMAC->Channel[0].CHCTRLA.reg = DMAC_CHCTRLA_BURSTLEN_SINGLE |           // Set up burst length to single burst
                                 DMAC_CHCTRLA_TRIGACT_BURST   |           // Trigger a transfer on burst
                                 DMAC_CHCTRLA_TRIGSRC(TCC1_DMAC_ID_MC_0); // Source is TCC cc0
  DMAC->Channel[0].CHPRILVL.bit.PRILVL  = DMAC_PRICTRL0_LVLPRI0_Pos;      // Priority is 0 (lowest)

  // Set up DMAC Channel 0 transfer descriptior (what is being transfered to memory)
  descriptor.descaddr = (uint32_t)&descriptor_section[0];               // Set up a circular descriptor
  descriptor.srcaddr = (uint32_t)&TCC1->CC[0].reg;                      // Take the contents of the TCC1 counter comapare 0 register
  descriptor.dstaddr = (uint32_t)&period;                           // Copy it to the "dmacPeriod" variable
  descriptor.btcnt = 1;                                                 // Transfer only takes 1 beat
  descriptor.btctrl = DMAC_BTCTRL_BEATSIZE_WORD | DMAC_BTCTRL_VALID;   // Copy 32-bits (WORD) and flag discriptor as valid
  memcpy(&descriptor_section[0], &descriptor, sizeof(dmacdescriptor));  // Copy to the channel 0

  // Set up DMAC Channel 1 /////////////////////////////////////////////////////////////////////
  DMAC->Channel[1].CHCTRLA.reg = DMAC_CHCTRLA_BURSTLEN_SINGLE |           // Set up burst length to single burst
                                 DMAC_CHCTRLA_TRIGACT_BURST   |           // Trigger a transfer on burst
                                 DMAC_CHCTRLA_TRIGSRC(TCC1_DMAC_ID_MC_1); // Source is TCC cc1
  DMAC->Channel[1].CHPRILVL.bit.PRILVL  = DMAC_PRICTRL0_LVLPRI0_Pos;      // Priority is 0 (lowest)

  // // Set up DMAC Channel 1 transfer descriptior (what is being transfered to memory)
  descriptor.descaddr = (uint32_t)&descriptor_section[1];               // Set up a circular descriptor
  descriptor.srcaddr = (uint32_t)&TCC1->CC[1].reg;                      // Take the contents of the TCC1 counter comapare 0 register
  descriptor.dstaddr = (uint32_t)&pulseWidth;                           // Copy it to the "dmacPeriod" variable
  descriptor.btcnt = 1;                                                 // Transfer only takes 1 beat
  descriptor.btctrl = DMAC_BTCTRL_BEATSIZE_WORD | DMAC_BTCTRL_VALID;   // Copy 32-bits (WORD) and flag discriptor as valid
  memcpy(&descriptor_section[1], &descriptor, sizeof(dmacdescriptor));  // Copy to the channel 1

  // configure TCC1 and A4
  GCLK->PCHCTRL[TCC1_GCLK_ID].reg = GCLK_PCHCTRL_CHEN |        // Enable the TCC1 perhipheral channel
                                    GCLK_PCHCTRL_GEN_GCLK0;    // Connect 120MHz generic clock 0 to TCC1

  PORT->Group[PORTA].PINCFG[4].bit.PMUXEN = 1;   // A4 is PA4
  PORT->Group[PORTA].PMUX[4 >> 1].reg |= PORT_PMUX_PMUXE(0);  //interrupt
  // PORT->Group[g_APinDescription[A4].ulPort].PINCFG[g_APinDescription[A4].ulPin].bit.PMUXEN = 1;
  // PORT->Group[g_APinDescription[A4].ulPort].PMUX[g_APinDescription[A4].ulPin >> 1].reg |= PORT_PMUX_PMUXE(0);
  // configure EIC interrupt
  EIC->CTRLA.bit.ENABLE = 0;                        // Disable the EIC peripheral
  while (EIC->SYNCBUSY.bit.ENABLE);                 // Wait for synchronization
  EIC->CONFIG[0].reg = EIC_CONFIG_SENSE4_HIGH;      // Set event on detecting a HIGH level
  EIC->EVCTRL.reg = 1 << 4;                         // Enable event output on external interrupt 4
  EIC->INTENCLR.reg = 1 << 4;                       // Clear interrupt on external interrupt 4
  EIC->ASYNCH.reg = 1 << 4;                         // Set-up interrupt as asynchronous input

  EIC->CTRLA.bit.ENABLE = 1;                        // Enable the EIC peripheral
  while (EIC->SYNCBUSY.bit.ENABLE);                 // Wait for synchronization

  MCLK->APBBMASK.reg |= MCLK_APBBMASK_EVSYS;         // Switch on the event system peripheral

  // Select the event system user on channel 0 (USER number = channel number + 1)
  EVSYS->USER[EVSYS_ID_USER_TCC1_EV_1].reg = EVSYS_USER_CHANNEL(1);         // Set the event user (receiver) as timer TCC1


  // Select the event system generator on channel 0
  EVSYS->Channel[0].CHANNEL.reg = EVSYS_CHANNEL_EDGSEL_NO_EVT_OUTPUT |              // No event edge detection
                                  EVSYS_CHANNEL_PATH_ASYNCHRONOUS |                 // Set event path as asynchronous
                                  EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_EIC_EXTINT_4);   // Set event generator (sender) as external interrupt 4

  // Set up TCC1 as a pwp
  TCC1->EVCTRL.reg |= TCC_EVCTRL_MCEI1 |           // Enable the match or capture channel 1 event input
                      TCC_EVCTRL_MCEI0 |           //.Enable the match or capture channel 0 event input
                      TCC_EVCTRL_TCEI1 |           // Enable the TCC event 1 input
                      /*TCC_EVCTRL_TCINV1 |*/      // Invert the event 1 input
                      TCC_EVCTRL_EVACT1_PPW;       // Set up the timer for capture: CC0 period, CC1 pulsewidth
#if 0
  TCC1->INTENSET.reg = TCC_INTENSET_MC1 |               // Enable compare channel 1 (CC1) interrupts
                       TCC_INTENSET_MC0;                // Enable compare channel 0 (CC0) interrupts
#endif
  TCC1->CTRLA.reg |= TCC_CTRLA_CPTEN1 |              // Enable capture on CC1
                     TCC_CTRLA_CPTEN0 |              // Enable capture on CC0
                     TCC_CTRLA_PRESCALER_DIV4;      // Set prescaler for 120mhz

  TCC1->CTRLA.bit.ENABLE = 0x1;
  while (TCC1->SYNCBUSY.bit.ENABLE); //Wait for TCC1 reg sync

  //Enabling DMAC channels
  DMAC->Channel[0].CHCTRLA.bit.ENABLE = 0x1;

  DMAC->Channel[1].CHCTRLA.bit.ENABLE = 0x1;
  Serial.println("Starting ");
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  delay(1000);
  Serial.println("go");
  //analogWrite(10,64);   //1831 hz  25% duty
  pwmWrite(120000, 60000);  // pin 10  div, duty  120mhz/120 1 mhz
  capture_init();
}

void loop() {
  Serial.print(period); Serial.print(" "); Serial.print(pulseWidth);
  Serial.print("  period "); Serial.print(period / 30.); // DIV4  120/4
  Serial.print(" us  pulse "); Serial.println(pulseWidth / 30.);
  delay(1000);
}
