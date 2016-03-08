#include <STM8S003F3P.h>
#include "w1209.h"

/// hs320281K
// Select
// 1(PD4) -> 12
// 11(PB5) -> 9
// 12(PB4) -> 8
// Segments
// 18(PD1) -> 1
// 20(PD3) -> 2
// 19(PD2) -> 3
// 17(PC7) -> 4
// 16(PC6) -> 5
// 6(PA2) -> 7
// 5(PA1) -> 10
// 2(PD5) -> 11

/// Keys
// SET 13(PC3)
// PLUS 14(PC4)
// MINUS 15(PC5)

// Relay 10(PA3)

// ADC 3 (PD6) AIN6

///----------------------------------------------------- Initialize -----------------------------------
void W1209Init(void)
{
	CLK_ICKR = 0;                       //  Reset the Internal Clock Register.
	CLK_ICKR = (1 << 0);                 //  Enable the HSI.
	CLK_ECKR = 0;                       //  Disable the external clock.
	while ((CLK_ICKR & (1 << 1)) == 0);       //  Wait for the HSI to be ready for use.
	CLK_CKDIVR = 0;                     //  Ensure the clocks are running at full speed.
	CLK_PCKENR1 = 0xff;                 //  Enable all peripheral clocks.
	CLK_PCKENR2 = 0xff;                 //  Ditto.
	CLK_CCOR = 0;                       //  Turn off CCO.
	CLK_HSITRIMR = 0;                   //  Turn off any HSIU trimming.
	CLK_SWIMCCR = 0;                    //  Set SWIM to run at clock / 2.
	CLK_SWR = 0xe1;                     //  Use HSI as the clock source.
	CLK_SWCR = 0;                       //  Reset the clock switch control register.
	CLK_SWCR = (1 << 1);                  //  Enable switching.
	while ((CLK_SWCR & (1 << 0)) != 0);        //  Pause while the clock switch is busy.

// All Pins to PullUp
	PA_CR1 = 0xff;
	PB_CR1 = 0xff;
	PC_CR1 = 0xff;
	PD_CR1 = 0xff;
	
	// All Pins to Slow mode
	PA_CR2 = 0;
	PB_CR2 = 0;
	PC_CR2 = 0;
	PD_CR2 = 0;
	
	//Configure direction
	PA_DDR = 0xFF;
	PB_DDR = 0xFF;
	PC_DDR = ~((1 << 3) | (1 << 4) | (1 << 5)); // Keys
	PD_DDR = ~(1 << 6); // AIN
	
	PC_ODR = (1 << 3) | (1 << 4) | (1 << 5); // Keys
	
	TIM2_CCMR1 = (1 << 3); // OC1PE: Output compare 1 preload enable
	TIM2_CNTRH = 0;
	TIM2_CNTRL = 0;
	TIM2_PSCR = 0x07; // 2^7 = 128, 16000000/128 = 125000 Hz

	// AutoReloadValue
	TIM2_ARRH = 0;
	TIM2_ARRL = 125; // 125000 Hz / 125 = 1000Hz,  1mlS

	TIM2_IER = (1 << 0); // UIE
	TIM2_CR1 = (1 << 0); // CEN: Counter enable
	enable_interrupts();
}

static volatile unsigned long gs_timer_counter = 0;

@far @interrupt void TimerInterruptHandler(void)
{
	TIM2_SR1 &= ~(1 << 0);
	gs_timer_counter++;
}

unsigned long GetCounter(void)
{
	unsigned long res1 = gs_timer_counter, res2 = gs_timer_counter;
	while(res1 != res2)
	{
		res1 = gs_timer_counter;
		res2 = gs_timer_counter;
	}
	return res1;
}

void Sleep(unsigned long mSec)
{
	unsigned long started = GetCounter();
	while((GetCounter() - started) < mSec);
}

///-------------------------------------------------------- ADC ---------------------------------------
short AdcGetValue(void)
{
	
}

///------------------------------------------------------ Indicator ------------------------------------
static volatile unsigned long gs_indicator_timer = 0;
static keys_t gs_keys;
static keys_t gs_last_keys;
static char gs_key_done;
static unsigned long gs_key_timer;

static unsigned char gs_indicator_pa[3];
static unsigned char gs_indicator_pb[3] = {3 << 4, 1 << 4, 1 << 5};
static unsigned char gs_indicator_pc[3];
static unsigned char gs_indicator_pd[3];
static unsigned char gs_indicator_index;
#define INDICATOR_POINT_MASK  (IND_POINT)
#define INDICATOR_MINUS_MASK  (IND_MIDDLE)

void W1209Step(void)
{
	keys_t cur_key = (((~PC_IDR) >> 3) & 0x07);
	unsigned long cur = GetCounter();
	if(cur_key != gs_last_keys)
	{
		if(cur_key == KEY_NONE)
		{
			if(!gs_key_done)
				gs_keys = gs_last_keys;
		}
		else
		{
			gs_key_done = 0;
			gs_key_timer = cur;
		}
		gs_last_keys = cur_key;
	}
	else if(cur_key != KEY_NONE && !gs_key_done)
	{
		if((cur - gs_key_timer) > 3000)
		{
			gs_keys = cur_key | KEY_LONG;
			gs_key_done = 1;
		}
	}
	if(cur - gs_indicator_timer >= 2)
	{
		gs_indicator_timer = cur;
		if(++gs_indicator_index > 2)
			gs_indicator_index = 0;
		PA_ODR = (PA_ODR & (~((1 << 1) | (1 << 2)))) | gs_indicator_pa[gs_indicator_index];
		PB_ODR = (PB_ODR & (~((1 << 4) | (1 << 5)))) | gs_indicator_pb[gs_indicator_index];
		PC_ODR = (PC_ODR & (~((1 << 6) | (1 << 7)))) | gs_indicator_pc[gs_indicator_index];
		PD_ODR = (PD_ODR & (~((1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5)))) | gs_indicator_pd[gs_indicator_index] | (gs_indicator_index == 0 ? 0 : (1 << 4));
}
}
// A1, A2, C6, C7, D1, D2, D3, D5
void SetIndicatorMap(int index, indicator_t map)
{
	if(index < 0 || index > 2)
		return;
	gs_indicator_pa[index] = (map & 0x03) << 1;
	gs_indicator_pc[index] = (map & 0x0C) << 4;
	gs_indicator_pd[index] = ((map & 0x70) >> 3) | ((map & 0x80) >> 2);
}

static unsigned char gs_indicator_digit[] = {
	IND_0,IND_1,IND_2,IND_3,IND_4,IND_5,IND_6,IND_7,IND_8,IND_9
};

void SetIndicatorValue(int index, unsigned char value, int point)
{
	if(value > 9)
		return;
	SetIndicatorMap(index, gs_indicator_digit[value] | (point ? IND_POINT : 0));
}
void SetIndicatorMSec(long value, int blinkPoint, ind_time_type_t type)
{
	int ival;
	if((type & IND_TIME_MS) && (value < 1000))
	{
		SetIndicatorInt(value, 0);
		return;
	}
	if((type & IND_TIME_SEC) && (value < 10000))
	{
		SetIndicatorInt(value/10, 2);
		return;
	}
	if((type & IND_TIME_MIN) && (value < 600000L))
	{
		value /= 1000;
		ival = (value / 60)*100 + (value % 60);
		SetIndicatorInt(ival, (GetCounter() % 1000) > 500 ? 2 : (blinkPoint ? 0 : 2));
		return;
	}
	value /= 60000;
	ival = (value / 60)*100 + (value % 60);
	SetIndicatorInt(ival, (GetCounter() % 1000) > 500 ? 2 : (blinkPoint ? 0 : 2));
}
void SetIndicatorInt(int value, int point)
{
	int ival;
	if(value < -199)
	{
		SetIndicatorMap(0, IND_L);
		SetIndicatorMap(1, IND_L);
		SetIndicatorMap(2, IND_L);
		return;
	}
	if(value > 999)
	{
		SetIndicatorMap(0, IND_H);
		SetIndicatorMap(1, IND_H);
		SetIndicatorMap(2, IND_H);
		return;
	}
	if(value <= -100)
	{
		ival = value % (-100);
		SetIndicatorMap(0, IND_MINUS_ONE);
		SetIndicatorValue(1, ival / 10, point == 1);
		SetIndicatorValue(2, ival % 10, 0);
		return;
	}
	if(value < 0)
	{
		ival = -value;
		SetIndicatorMap(0, IND_MINUS);
		SetIndicatorValue(1, ival / 10, point == 1);
		SetIndicatorValue(2, ival % 10, 0);
		return;
	}
	SetIndicatorValue(0, value / 100, point == 2);
	SetIndicatorValue(1, (ival % 100) / 10, point == 1);
	SetIndicatorValue(2, ival % 10, 0);
}

void SetIndicatorFloat(float value)
{
	int ival;
	if(value < -199)
	{
		SetIndicatorMap(0, IND_L);
		SetIndicatorMap(1, IND_L);
		SetIndicatorMap(2, IND_L);
		return;
	}
	if(value > 999)
	{
		SetIndicatorMap(0, IND_H);
		SetIndicatorMap(1, IND_H);
		SetIndicatorMap(2, IND_H);
		return;
	}
	if(value <= -100)
	{
		SetIndicatorMap(0, IND_MINUS_ONE);
		ival = ((int)value)/(-10);
		SetIndicatorValue(1, ival / 10, 0);
		SetIndicatorValue(2, ival % 10, 0);
		return;
	}
	if(value <= -10)
	{
		SetIndicatorMap(0, IND_MINUS);
		ival = -value;
		SetIndicatorValue(1, ival / 10, 0);
		SetIndicatorValue(2, ival % 10, 0);
		return;
	}
	if(value < 0)
	{
		SetIndicatorMap(0, IND_MINUS);
		ival = value*(-10);
		SetIndicatorValue(1, ival / 10, 1);
		SetIndicatorValue(2, ival % 10, 0);
		return;
	}
	if(value >= 100)
	{
		ival = value;
		SetIndicatorValue(0, ival / 100, 0);
		SetIndicatorValue(1, (ival % 100) / 10, 0);
		SetIndicatorValue(2, ival % 10, 0);
		return;
	}
	ival = value*10;
	SetIndicatorValue(0, ival / 100, 0);
	SetIndicatorValue(1, (ival % 100) / 10, 1);
	SetIndicatorValue(2, ival % 10, 0);
}


///---------------------------------------------------------- Keys ------------------------------------
keys_t GetKeys(void)
{
	keys_t res = gs_keys;
	gs_keys = KEY_NONE;
	//return (PC_IDR >> 3) & 0x07;
}

int IsKeyPressed(keys_t key)
{
	return (gs_keys & key) != 0;
}



///---------------------------------------------------------- Relay ------------------------------------
void SetRelayOn(void) 
{
	SetRelayState(1);
}

void SetRelayOff(void) 
{
	SetRelayState(0);
}
void SetRelayState(int value) 
{
	if(value) PA_ODR |= (1 << 3);
	else PA_ODR &= ~(1 << 3);
}

int GetRelayState(void) 
{
	return (PA_ODR & (1 << 3)) != 0;
}
