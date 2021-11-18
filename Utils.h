#pragma once
#include "Common.h"

DWORD GetEventInfoString(EVT_HANDLE hEvent, LPWSTR* evtContent);

DWORD WINAPI DumpPreviousUSBList(LPCWSTR &lpUsbListPath);

string parseInstanceId(string &fullId);

string LPCWSTRToSTR(LPCWSTR ptr);

void SendEmail(unsigned int state);

