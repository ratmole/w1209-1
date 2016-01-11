#pragma once

typedef struct {
	int check1;
	int size;
	int calibr_0;
	int calibr_100;
	float pid_level;
	unsigned int pid_default_percent;
	unsigned int pid_mlsec;
	float pid_P;
	float pid_I;
	float pid_D;
	float pid_I_limit;
	unsigned int pwm_mlsec;
	int check2;
} settings_t;

extern @eeprom settings_t g_settings;