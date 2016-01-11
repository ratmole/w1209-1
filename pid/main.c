#include <w1209.h>
#include "settings.h"
#include <STM8S003F3P.h>

#define SETTINGS_CHECK_VALUE 0xA5

@eeprom settings_t g_settings;

static float gs_current_temperature = 20;

static unsigned char gs_menu_level;
static unsigned char gs_menu_param;
static unsigned long gs_menu_timer;
#define MAX_MENU 8

void MenuFloatOnKey(keys_t key, @eeprom float* param, float delta, float minValue, float maxValue)
{
	float val;
	if(key == KEY_SET)
		gs_menu_level = key & KEY_LONG ? 0 : 1;
	else if(key == KEY_PLUS)
	{
		val = *param + delta;
		if(val > maxValue) val = maxValue;
		if(*param != val)
			*param = val;
	}
	else if(key == KEY_MINUS)
	{
		val = *param - delta;
		if(val < minValue) val = minValue;
		if(*param != val)
			*param = val;
	}
}

void MenuUIntOnKey(keys_t key, @eeprom unsigned int* param, unsigned int delta, unsigned int minValue, unsigned int maxValue)
{
	unsigned int val;
	if(key == KEY_SET)
		gs_menu_level = key & KEY_LONG ? 0 : 1;
	else if(key == KEY_PLUS)
	{
		val = *param + delta;
		if(val > maxValue) val = maxValue;
		if(*param != val)
			*param = val;
	}
	else if(key == KEY_MINUS)
	{
		val = *param - delta;
		if(val < minValue) val = minValue;
		if(*param != val)
			*param = val;
	}
}

void MenuOnKey(void)
{
	keys_t cur_key = GetKeys();
	unsigned long cur = GetCounter();
	if(cur_key == KEY_NONE && gs_menu_level != 0)
	{
		if((cur - gs_menu_timer) > 5000)
			gs_menu_level = 0;
		return;
	}
	gs_menu_timer = cur;
	switch(gs_menu_level)
	{
	case 1:
	{
		if(cur_key & KEY_SET)
			gs_menu_level = cur_key & KEY_LONG ? 0 : 2;
		else if(cur_key == KEY_MINUS)
		{
			if(--gs_menu_param > MAX_MENU) gs_menu_param = MAX_MENU;
		}
		else if(cur_key == KEY_PLUS)
		{
			if(--gs_menu_param >= MAX_MENU) gs_menu_param = 0;
		}
		break;
	}
	case 2:
	{
		switch(gs_menu_param)
		{
			case 0:
				MenuFloatOnKey(cur_key, &g_settings.pid_level, 0.1, 0, 100);
				break;
			case 1:
				MenuUIntOnKey(cur_key, &g_settings.pid_default_percent, 1, 0, 100);
				break;
			case 2:
				MenuUIntOnKey(cur_key, &g_settings.pid_mlsec, 100, 100, 60000);
				break;
			case 3:
				MenuFloatOnKey(cur_key, &g_settings.pid_P, 0.1, 0.1, 100);
				break;
			case 4:
				MenuFloatOnKey(cur_key, &g_settings.pid_I, 0.1, 0.1, 1);
				break;
			case 5:
				MenuFloatOnKey(cur_key, &g_settings.pid_D, 0.1, 0.1, 10);
				break;
			case 6:
				MenuFloatOnKey(cur_key, &g_settings.pid_I_limit, 1, 1, 40);
				break;
			case 7:
				MenuUIntOnKey(cur_key, &g_settings.pwm_mlsec, 100, 100, 60000);
				break;
		}
		break;
	}
	default:
		if(cur_key & KEY_SET)
		{
			if(cur_key & KEY_LONG)
			{
				gs_menu_level = 1;
			}
			else
			{
				//gs_last_ticks = GetCounter();
			}
		}
	break;
	}
}

void Menu_Step(void)
{
	MenuOnKey();
	switch(gs_menu_level)
	{
	case 1:
	{
		SetIndicatorMap(0, IND_NONE);
		SetIndicatorMap(1, IND_T);
		SetIndicatorMap(2, gs_menu_param ? IND_1 : IND_0);
		break;
	}
/*	case 2:
	{       
		if(gs_menu_param)
			SetIndicatorMSec(g_settings.on_mlSec, 0, IND_TIME_MIN_SEC);
		else
			SetIndicatorMSec(60000L * g_settings.standby_minutes, 0, IND_TIME_MIN_SEC);
		break;
	}
	default:
	{
		unsigned long need = (gs_state ? (unsigned long)g_settings.on_mlSec : 60000L * g_settings.standby_minutes);
		unsigned long count = GetCounter() - gs_last_ticks;
		SetIndicatorMSec(count > need ? 0L : need - count, 1, IND_TIME_MIN_SEC);
		break;
	}*/
	}
}



void Adc_Step(void)
{
	//gs_current_temperature
}



static float gs_current_percent;
static float gs_last_temperature;
static float gs_intergrator_value;
static unsigned long gs_pid_timer;

void Pid_Step(void)
{
	float pid_value;
	unsigned long cur = GetCounter();
	float delta_temperature = gs_current_temperature - g_settings.pid_level; 
	if((cur - gs_pid_timer) < g_settings.pid_mlsec)
		return;
	gs_pid_timer = cur;
	gs_intergrator_value += delta_temperature*g_settings.pid_I;
	if(gs_intergrator_value > g_settings.pid_I_limit)
		gs_intergrator_value = g_settings.pid_I_limit;
	pid_value = gs_current_percent + g_settings.pid_P*delta_temperature + gs_intergrator_value + g_settings.pid_D*delta_temperature;
	gs_current_percent = (pid_value > 100) ? 100 : pid_value;
}

static unsigned long gs_pwm_timer;

void Pwm_Step(void)
{
	unsigned long cur = GetCounter();
	unsigned long delta_timer = cur - gs_pwm_timer;
	unsigned int vkl_time = gs_current_percent <= 0 ? 0 :(gs_current_percent >= 100 ? g_settings.pwm_mlsec : gs_current_percent * g_settings.pwm_mlsec / 100);
	if(delta_timer >= g_settings.pwm_mlsec)
	{
		delta_timer = 0;
		gs_pwm_timer = cur;
	}
	SetRelayState(delta_timer < vkl_time);
}


main()
{
	W1209Init();
	FLASH_DUKR = 0xAE;
	FLASH_DUKR = 0x56;
	if(g_settings.check1 != SETTINGS_CHECK_VALUE 
		|| g_settings.check1 != SETTINGS_CHECK_VALUE
		|| g_settings.size != sizeof(settings_t))
	{
		g_settings.calibr_0 = 0;
		g_settings.calibr_100 = 100;
		g_settings.pid_level = 37.8;
		// Time on - 1m 47sec (107sec), time off - 4m 13sec(253sec), all time 6m(360sec)
		g_settings.pwm_mlsec = 60000;
		g_settings.pid_default_percent = 30;
		g_settings.pid_mlsec = 60000;
		// 100% for 20 degrees, 30% for 40 degres = (100 - 30) / (40-20) = 70/20=3.5
		g_settings.pid_P = 3.5;
		g_settings.pid_I = 0;
		g_settings.pid_D = 0;
		g_settings.pid_I_limit = 0;
		
		g_settings.check1 = SETTINGS_CHECK_VALUE;
		g_settings.check2 = SETTINGS_CHECK_VALUE;
		g_settings.size = sizeof(settings_t);
	}
	gs_current_percent = g_settings.pid_default_percent;
	for(;;)
	{
		W1209Step();
		Adc_Step();
		Pid_Step();
		Pwm_Step();
		Menu_Step();
	}
}