#pragma once
#include "Common.h"
class SystemState
{
public:
	unsigned int system_state;
	bool is_locked;
	bool unknown_usb;
	bool unknown_wifi;
	bool abusable_process;
	bool usb_debug_mode;
	
	SystemState() : system_state(0), is_locked(false), unknown_usb(false), unknown_wifi(false), abusable_process(false), usb_debug_mode(false){}

	void CheckSystemState();
};

