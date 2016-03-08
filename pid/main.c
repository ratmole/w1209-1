#include <w1209.h>
#include "settings.h"
#include <STM8S003F3P.h>

#define SETTINGS_CHECK_VALUE 0xA5

@eeprom settings_t g_settings;

static float gs_current_temperature;
static unsigned short gs_current_adc;

static unsigned char gs_menu_level;
static unsigned char gs_menu_param;
static unsigned char gs_menu_param_2;
static unsigned long gs_menu_timer;
enum menu_level_t
{
	MENU_PID_LEVEL = 0,
	MENU_PID_DEFAULT_PERCENT,
	MENU_PID_MLSEC,
	MENU_PID_P,
	MENU_PID_I,
	MENU_PID_D,
	MENU_PID_I_LIMIT,
	MUNU_PWM_MLSEC,
	MENU_CALIBR,
	MENU_END
};

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
			if(--gs_menu_param >= MENU_END) gs_menu_param = MENU_END-1;
		}
		else if(cur_key == KEY_PLUS)
		{
			if(++gs_menu_param >= MENU_END) gs_menu_param = 0;
		}
		break;
	}
	case 2:
	{
		switch(gs_menu_param)
		{
			case MENU_PID_LEVEL:
				MenuFloatOnKey(cur_key, &g_settings.pid_level, 0.1, 0, 100);
				break;
			case MENU_PID_DEFAULT_PERCENT:
				MenuUIntOnKey(cur_key, &g_settings.pid_default_percent, 1, 0, 100);
				break;
			case MENU_PID_MLSEC:
				MenuUIntOnKey(cur_key, &g_settings.pid_mlsec, 100, 100, 60000);
				break;
			case MENU_PID_P:
				MenuFloatOnKey(cur_key, &g_settings.pid_P, 0.1, 0.1, 100);
				break;
			case MENU_PID_I:
				MenuFloatOnKey(cur_key, &g_settings.pid_I, 0.1, 0.1, 1);
				break;
			case MENU_PID_D:
				MenuFloatOnKey(cur_key, &g_settings.pid_D, 0.1, 0.1, 10);
				break;
			case MENU_PID_I_LIMIT:
				MenuFloatOnKey(cur_key, &g_settings.pid_I_limit, 1, 1, 40);
				break;
			case MUNU_PWM_MLSEC:
				MenuUIntOnKey(cur_key, &g_settings.pwm_mlsec, 100, 100, 60000);
				break;
			case MENU_CALIBR:
				if(cur_key == KEY_MINUS || cur_key == KEY_PLUS)
					gs_menu_param_2 = !gs_menu_param_2;
				else if(cur_key == KEY_SET)
				{
					if(gs_menu_param_2)
						g_settings.calibr_100 = gs_current_adc;
					else
						g_settings.calibr_0 = gs_current_adc;
				}
				break;
		}
		break;
	}
	default:
		if(cur_key & (KEY_SET | KEY_LONG))
		{
			gs_menu_level = 1;
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
		switch(gs_menu_param)
		{
			case MENU_PID_LEVEL:
				SetIndicatorMap(0, IND_L);
				SetIndicatorMap(1, IND_E);
				SetIndicatorMap(2, IND_V);
				break;
			case MENU_PID_DEFAULT_PERCENT:
				SetIndicatorMap(0, IND_L);
				SetIndicatorMap(1, IND_E);
				SetIndicatorMap(2, IND_E);
				break;
			case MENU_PID_MLSEC:
				SetIndicatorMap(0, IND_P);
				SetIndicatorMap(1, IND_T);
				SetIndicatorMap(2, IND_I);
				break;
			case MENU_PID_P:
				SetIndicatorMap(0, IND_P);
				SetIndicatorMap(1, IND__);
				SetIndicatorMap(2, IND_P);
				break;
			case MENU_PID_I:
				SetIndicatorMap(0, IND_P);
				SetIndicatorMap(1, IND__);
				SetIndicatorMap(2, IND_I);
				break;
			case MENU_PID_D:
				SetIndicatorMap(0, IND_P);
				SetIndicatorMap(1, IND__);
				SetIndicatorMap(2, IND_O);
				break;
			case MENU_PID_I_LIMIT:
				SetIndicatorMap(0, IND_P);
				SetIndicatorMap(1, IND_I);
				SetIndicatorMap(2, IND_L);
				break;
			case MUNU_PWM_MLSEC:
				SetIndicatorMap(0, IND_P);
				SetIndicatorMap(1, IND_P);
				SetIndicatorMap(2, IND_T);
				break;
			case MENU_CALIBR:
				SetIndicatorMap(0, IND_C);
				SetIndicatorMap(1, IND_A);
				SetIndicatorMap(2, IND_L);
				break;
		}
		break;
	}
	case 2:
	{       
		switch(gs_menu_param)
		{
			case MENU_PID_LEVEL:
				SetIndicatorFloat(g_settings.pid_level);
				break;
			case MENU_PID_DEFAULT_PERCENT:
				SetIndicatorInt(g_settings.pid_default_percent, -1);
				break;
			case MENU_PID_MLSEC:
				SetIndicatorInt(g_settings.pid_mlsec, -1);
				break;
			case MENU_PID_P:
				SetIndicatorFloat(g_settings.pid_P);
				break;
			case MENU_PID_I:
				SetIndicatorFloat(g_settings.pid_I);
				break;
			case MENU_PID_D:
				SetIndicatorFloat(g_settings.pid_D);
				break;
			case MENU_PID_I_LIMIT:
				SetIndicatorFloat(g_settings.pid_I_limit);
				break;
			case MUNU_PWM_MLSEC:
				SetIndicatorInt(g_settings.pwm_mlsec, -1);
				break;
			case MENU_CALIBR:
				SetIndicatorMap(0, IND_C);
				SetIndicatorMap(1, IND_L);
				SetIndicatorMap(2, gs_menu_param_2 ? IND_1 : IND_0);
				break;
		}
		break;
	}
	default:
		SetIndicatorFloat(gs_current_temperature);
		break;
	}
}


void Adc_Step(void)
{
	float fval, val;
	
	fval = (gs_current_adc - g_settings.calibr_0);
	val /= (g_settings.calibr_100 - g_settings.calibr_0);
	val *= 100;
	gs_current_temperature = val;
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
		g_settings.pid_level = 38;
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