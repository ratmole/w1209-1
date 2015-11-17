#include <w1209.h>
#include "settings.h"
#include <STM8S003F3P.h>

#define SETTINGS_CHECK_VALUE 0xA6

@eeprom settings_t g_settings;

main()
{
	W1209Init();
	FLASH_DUKR = 0xAE;
	FLASH_DUKR = 0x56;
	if(g_settings.check1 != SETTINGS_CHECK_VALUE 
		|| g_settings.check1 != SETTINGS_CHECK_VALUE
		|| g_settings.size != sizeof(settings_t))
	{
		g_settings.calibr10v = 10;
		g_settings.calibr14v = 14;
		g_settings.minLevel = 10.7;
		g_settings.maxLevel = 15;
		g_settings.restoreLevel = 11.7;
		g_settings.check1 = SETTINGS_CHECK_VALUE;
		g_settings.check2 = SETTINGS_CHECK_VALUE;
		g_settings.size = sizeof(settings_t);
	}
	for(;;)
	{
	}
}