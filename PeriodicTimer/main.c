#include <w1209.h>
#include "settings.h"
#include <STM8S003F3P.h>

#define SETTINGS_CHECK_VALUE 0xA5

@eeprom settings_t g_settings;

static unsigned long gs_last_ticks;
static unsigned char gs_state;


void Relay_Step(void)
{
	if((GetCounter() - gs_last_ticks) >= (gs_state ? (unsigned long)g_settings.on_mlSec : 60000L * g_settings.standby_minutes))
	{
		gs_last_ticks = GetCounter();
		gs_state = !gs_state;
	}
	SetRelayState(gs_state);
}

static unsigned char gs_menu_level;
static unsigned char gs_menu_param;
static unsigned long gs_menu_timer;

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
		else if(cur_key == KEY_PLUS || cur_key == KEY_MINUS)
			gs_menu_param = !gs_menu_param;
		break;
	}
	case 2:
	{
		if(cur_key == KEY_SET)
			gs_menu_level = cur_key & KEY_LONG ? 0 : 1;
		else if(cur_key == KEY_PLUS)
		{
			if(gs_menu_param)
			{
				if(g_settings.on_mlSec < 5000)
					g_settings.on_mlSec += 100;
			}
			else
			{
				if(g_settings.standby_minutes < 10*60)
					g_settings.standby_minutes += 10;
			}
		}
		else if(cur_key == KEY_MINUS)
		{
			if(gs_menu_param)
			{
				if(g_settings.on_mlSec > 100)
					g_settings.on_mlSec -= 100;
			}
			else
			{
				if(g_settings.standby_minutes > 10)
					g_settings.standby_minutes -= 10;
			}
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
				gs_last_ticks = GetCounter();
				gs_state = 1;
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
		SetIndicatorMap(1, IND_P);
		SetIndicatorMap(2, gs_menu_param ? IND_1 : IND_0);
		break;
	}
	case 2:
	{
		if(gs_menu_param)
			SetIndicatorMSec(g_settings.on_mlSec, 0);
		else
			SetIndicatorMSec(60000L * g_settings.standby_minutes, 0);
		break;
	}
	default:
	{
		unsigned long need = (gs_state ? (unsigned long)g_settings.on_mlSec : 60000L * g_settings.standby_minutes);
		unsigned long count = GetCounter() - gs_last_ticks;
		SetIndicatorMSec(count > need ? 0L : need - count, 1);
		break;
	}
	}
}



void main(void)
{
	W1209Init();
	FLASH_DUKR = 0xAE;
	FLASH_DUKR = 0x56;
	if(g_settings.check1 != SETTINGS_CHECK_VALUE 
		|| g_settings.check1 != SETTINGS_CHECK_VALUE
		|| g_settings.size != sizeof(settings_t))
	{
		g_settings.standby_minutes = 4*60;
		g_settings.on_mlSec = 1000;
		g_settings.check1 = SETTINGS_CHECK_VALUE;
		g_settings.check2 = SETTINGS_CHECK_VALUE;
		g_settings.size = sizeof(settings_t);
	}

	Sleep(500);
	gs_state = 1;
	gs_last_ticks = GetCounter();
	SetIndicatorMap(0, IND_L);
	SetIndicatorMap(1, IND_0);
	SetIndicatorMap(2, IND_H);
	for(;;)
	{
		W1209Step();
		Relay_Step();
		Menu_Step();
	}
}