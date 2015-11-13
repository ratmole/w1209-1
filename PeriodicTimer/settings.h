#pragma once

typedef struct {
	int check1;
	int size;
	int standby_minutes; 
	int on_mlSec;
	int check2;
} settings_t;

extern @eeprom settings_t g_settings;