#include "MK64F12.h"

#define MASK(x) (1UL << (x))       

#define DATA_IS_PTC12 1

typedef unsigned char byte;

void setup_pins(void);
void shiftOutMSB(byte value);
void writeDigit(byte position, byte segData);
void showNumber(int n);
void delayMs(int n);

void set_led(void);
void set_sw(void);
void chack_button();

int num = 0;
int state = 0;

#define LATCH_PIN 23                /* PTB23 */
#define CLK_PIN   3                 /* PTC3  */

#define DATA_PIN  12            /* PTC12 */
#define DATA_PT   PTC
#define DATA_PORT PORTC

#define LATCH_HIGH() (PTB->PSOR = MASK(LATCH_PIN))
#define LATCH_LOW()  (PTB->PCOR = MASK(LATCH_PIN))
#define CLK_HIGH()   (PTC->PSOR = MASK(CLK_PIN))
#define CLK_LOW()    (PTC->PCOR = MASK(CLK_PIN))
#define DATA_HIGH()  (DATA_PT->PSOR = MASK(DATA_PIN))
#define DATA_LOW()   (DATA_PT->PCOR = MASK(DATA_PIN))

const byte SEGMENTS[10] = {
	
	0xC0, /* 0 */
	0xF9, /* 1 */
	0xA4, /* 2 */
	0xB0, /* 3 */
	0x99, /* 4 */
	0x92, /* 5 */
	0x82, /* 6 */
	0xF8, /* 7 */
	0x80, /* 8 */
	0x90  /* 9 */
	
};

const byte SEG_BLANK = 0xFF;  
const byte DIGIT_SELECT[4] = { 0xF1, 0xF2, 0xF4, 0xF8 };

int main (void) {

	int number = 0;              
	setup_pins();                   
	set_sw();
	set_led();
	
	while (1) {                     
		showNumber(num); 
	}
}


void setup_pins(void) {

	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTC_MASK;

	PORTA->PCR[1] = PORT_PCR_MUX(1); // busser
	
	PORTB->PCR[LATCH_PIN]    = PORT_PCR_MUX(1);
	PORTC->PCR[CLK_PIN]      = PORT_PCR_MUX(1);
	DATA_PORT->PCR[DATA_PIN] = PORT_PCR_MUX(1);

	PTB->PDDR |= MASK(LATCH_PIN);
	PTC->PDDR |= MASK(CLK_PIN);
	DATA_PT->PDDR |= MASK(DATA_PIN);

	LATCH_LOW();
	CLK_LOW();
	DATA_LOW();
	
}

void shiftOutMSB(byte value) {

	int i;

	for (i = 7; i >= 0; i--) {
		if (value & MASK(i))       
			DATA_HIGH();
		else
			DATA_LOW();

		CLK_HIGH();                 
		__NOP(); __NOP();          
		CLK_LOW();                  
		__NOP(); __NOP();
	}
}

void writeDigit(byte position, byte segData) {

	LATCH_LOW();                            
	shiftOutMSB(segData);                   
	shiftOutMSB(DIGIT_SELECT[position]);   
	LATCH_HIGH();
	
}

void showNumber(int n) {

	byte seg[4];
	int position;

	if (n < 0)    n = 0;
	if (n > 9999) n = 9999;

	seg[3] = SEGMENTS[n % 10];                                  
	seg[2] = (n >= 10)   ? SEGMENTS[(n / 10) % 10]   : SEG_BLANK;   
	seg[1] = (n >= 100)  ? SEGMENTS[(n / 100) % 10]  : SEG_BLANK;  
	seg[0] = (n >= 1000) ? SEGMENTS[(n / 1000) % 10] : SEG_BLANK;   

	for (position = 0; position < 4; position++) {
		writeDigit(position, seg[position]);
	}
}


void set_led(void){
	SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
	
	PORTD->PCR[1] = PORT_PCR_MUX(1); //led_1
	PORTD->PCR[3] = PORT_PCR_MUX(1); //led_2
	PORTD->PCR[2] = PORT_PCR_MUX(1); //led_3
	PORTD->PCR[0] = PORT_PCR_MUX(1); //led_4
	
	PTD->PDDR |= MASK(1);
	PTD->PDDR |= MASK(3);
	PTD->PDDR |= MASK(2);
	PTD->PDDR |= MASK(0);
	
	PTD->PDOR &= ~MASK(1);
	PTD->PDOR |= MASK(3);
	PTD->PDOR |= MASK(2);
	PTD->PDOR |= MASK(0);
}

void set_sw(void){
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
	
	PORTB->PCR[3] |= PORT_PCR_MUX(1); //sw1
	PORTB->PCR[10] |= PORT_PCR_MUX(1); //sw2
	PORTB->PCR[11] |= PORT_PCR_MUX(1); //sw3
	
	PTB->PDDR &= ~(MASK(3) | MASK(10) | MASK(11));
	
	__disable_irq();
	
	PORTB->PCR[3] &= ~PORT_PCR_IRQC_MASK; 
	PORTB->PCR[3] |= PORT_PCR_IRQC(0xA);
	
	PORTB->PCR[10] &= ~PORT_PCR_IRQC_MASK; 
	PORTB->PCR[10] |= PORT_PCR_IRQC(0xA);
	
	PORTB->PCR[11] &= ~PORT_PCR_IRQC_MASK; 
	PORTB->PCR[11] |= PORT_PCR_IRQC(0xA);
	
	NVIC_EnableIRQ(PORTB_IRQn); 
	
	__enable_irq();
	
	
}

void PORTB_IRQHandler(void) {
	chack_button();
	PORTB->ISFR = MASK(3); /* Step clear interrupt flag */
	PORTB->ISFR = MASK(10);
	PORTB->ISFR = MASK(11);
}

void chack_button(){
	if (PTB->PDIR == 0xC00){
			
			state = (state + 1) % 4;
			
			if(state == 0){
				PTD->PDOR = 0xD;
			}
			else if (state == 1){
				PTD->PDOR = 0x7;
			}
			else if (state == 2){
				PTD->PDOR = 0xB;
			}
			else if (state == 3){
				PTD->PDOR = 0xE;
			}
			while (!(PTB->PDIR & MASK(3)));
			delayMs(150);
			
		}
	else if (PTB->PDIR == 0x808){
			
			if(state == 0){
				num++;
			}
			else if (state == 1){
				num += 10;
			}
			else if (state == 2){
				num += 100;
			}
			else if (state == 3){
				num += 1000;
			}
			
			if (num > 9999){
				num = 0;
			}
			
			while (!(PTB->PDIR & MASK(10)));
			delayMs(150);
		}
	else if (PTB->PDIR == 0x408){
			num = 0;
			PTA->PDDR |= MASK(1);
			delayMs(100);
			PTA->PDDR &= ~MASK(1);

			while (!(PTB->PDIR & MASK(11)));
			delayMs(150);
		}
}

void delayMs(int n) {
	int i;
	SysTick->LOAD = 20480 - 1;
	/* Enable the timer and choose sysclk as the clock source */
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
	for (i = 0; i < n; i++)
		while((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0) { /* wait until the
		COUNT flag is set */
		}
	SysTick->CTRL = 0; /* Stop the timer (Enable = 0) */
}