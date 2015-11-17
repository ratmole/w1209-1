#pragma once

typedef struct {
	int check1;
	int size;
	int calibr10v;
	int calibr14v;
	float minLevel;
	float maxLevel;
	float restoreLevel;
	int check2;
} settings_t;

extern @eeprom settings_t g_settings;