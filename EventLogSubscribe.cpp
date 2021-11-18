#include "Utils.h"
#include "SystemState.h"
#include <vector>
using namespace std;



#pragma comment(lib, "wevtapi.lib")

#define QUERY_LOCKED \
    L"<QueryList>" \
    L"  <Query Path='Security'>" \
    L"    <Select>Event[System[EventID=4800]]</Select>" \
    L"  </Query>" \
    L"</QueryList>"

#define QUERY_UNLOCKED \
    L"<QueryList>" \
    L"  <Query Path='Security'>" \
    L"    <Select>Event[System[EventID=4801]]</Select>" \
    L"  </Query>" \
    L"</QueryList>"

#define QUERY_CONNECTED_WIFI \
    L"<QueryList>" \
    L"  <Query Path='Microsoft-Windows-NetworkProfile/Operational'>" \
    L"    <Select>Event[System[EventID=10000] and EventData[Data[@Name='Name']!='Identifying...']]</Select>" \
    L"  </Query>" \
    L"</QueryList>"

#define QUERY_CONNECTED_STORAGE_DEVICE \
    L"<QueryList>" \
    L"  <Query Path='Microsoft-Windows-DriverFrameworks-UserMode/Operational'>" \
    L"    <Select>Event[System[EventID=2003]]</Select>" \
    L"  </Query>" \
    L"</QueryList>"

// The structured XML query.
#define QUERY_PROCESS_OPENED \
    L"<QueryList>" \
    L"  <Query Path='Security'>" \
    L"    <Select>Event[System[EventID=4688] and EventData[Data[@Name='NewProcessName']='C:\\Windows\\System32\\Narrator.exe']]</Select>" \
    L"  </Query>" \
    L"</QueryList>"


DWORD WINAPI LockCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent);
DWORD WINAPI UnLockCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent);
DWORD WINAPI ConnectedWifiCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent);
DWORD WINAPI ConnectedStorageDeviceCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent);
DWORD WINAPI AbusableProcessOpenedCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent);

SystemState systemState;
LPCWSTR lpUsbListPathW = L"USBDevList.txt";



void main(void)
{
	DWORD status = ERROR_SUCCESS;
	EVT_HANDLE hLockSubscription = NULL;

	//systemState.usb_debug_mode = false;

	// Subscribe to events beginning with the oldest event in the channel. The subscription
	// will return all current events in the channel and any future events that are raised
	// while the application is active.
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~ SUBSCRIBING LOCK ~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
	hLockSubscription = EvtSubscribe(NULL, NULL, NULL, QUERY_LOCKED, NULL, NULL,
		(EVT_SUBSCRIBE_CALLBACK)LockCallback, EvtSubscribeToFutureEvents);
	if (NULL == hLockSubscription)
	{
		status = GetLastError();

		if (ERROR_EVT_CHANNEL_NOT_FOUND == status)
			wprintf(L"Channel %s was not found.\n", QUERY_LOCKED);
		else if (ERROR_EVT_INVALID_QUERY == status)
			// You can call EvtGetExtendedStatus to get information as to why the query is not valid.
			wprintf(L"The query \"%s\" is not valid.\n", QUERY_LOCKED);
		else
			wprintf(L"EvtSubscribe failed with %lu.\n", status);

		goto cleanup;
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~ DONE SUBSCRIBING! ~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

	wprintf(L"Waiting for LOCK to start monitoring\n\n");
	while (!_kbhit())
		Sleep(10);

cleanup:

	if (hLockSubscription)
		EvtClose(hLockSubscription);

}

// The callback that receives the events that match the query criteria. 
DWORD WINAPI LockCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent)
{
	DWORD status = ERROR_SUCCESS;
	EVT_HANDLE hUnlockSubscription = NULL;
	EVT_HANDLE hConnectedWifiSubscription = NULL;
	EVT_HANDLE hConnectedStorageDeviceSubscription = NULL;
	EVT_HANDLE hProcessOpenedSubscription = NULL;
	LPWSTR pRenderedContent = nullptr;

	auto start = std::chrono::system_clock::now();
	auto legacyStart = std::chrono::system_clock::to_time_t(start);

	UNREFERENCED_PARAMETER(pContext);

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~ Getting USB History From Registry ~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

	status = DumpPreviousUSBList(lpUsbListPathW);

	if (status != ERROR_SUCCESS) {
		wprintf(L"Failed to get USB History.\n");
		goto cleanup;
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~ SUBSCRIBING UNLOCK ~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

	hUnlockSubscription = EvtSubscribe(NULL, NULL, NULL, QUERY_UNLOCKED, NULL, NULL,
		(EVT_SUBSCRIBE_CALLBACK)UnLockCallback, EvtSubscribeToFutureEvents);
	if (NULL == hUnlockSubscription)
	{
		status = GetLastError();

		if (ERROR_EVT_CHANNEL_NOT_FOUND == status)
			wprintf(L"Channel %s was not found.\n", QUERY_UNLOCKED);
		else if (ERROR_EVT_INVALID_QUERY == status)
			// You can call EvtGetExtendedStatus to get information as to why the query is not valid.
			wprintf(L"The query \"%s\" is not valid.\n", QUERY_UNLOCKED);
		else
			wprintf(L"EvtSubscribe hUnlockSubscription failed with %lu.\n", status);

		goto cleanup;
	}
	else {
		wprintf(L"hUnlockSubscription done.\n");
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~ SUBSCRIBING CONNECTED WIFI ~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

	hConnectedWifiSubscription = EvtSubscribe(NULL, NULL, NULL, QUERY_CONNECTED_WIFI, NULL, NULL,
		(EVT_SUBSCRIBE_CALLBACK)ConnectedWifiCallback, EvtSubscribeToFutureEvents);
	if (NULL == hConnectedWifiSubscription)
	{
		status = GetLastError();

		if (ERROR_EVT_CHANNEL_NOT_FOUND == status)
			wprintf(L"Channel %s was not found.\n", QUERY_CONNECTED_WIFI);
		else if (ERROR_EVT_INVALID_QUERY == status)
			// You can call EvtGetExtendedStatus to get information as to why the query is not valid.
			wprintf(L"The query \"%s\" is not valid.\n", QUERY_CONNECTED_WIFI);
		else
			wprintf(L"EvtSubscribe hConnectedWifiSubscription failed with %lu.\n", status);

		goto cleanup;
	}
	else {
		wprintf(L"hConnectedWifiSubscription done.\n");
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~ SUBSCRIBING CONNECTED STORAGE DEVICE ~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

	hConnectedStorageDeviceSubscription = EvtSubscribe(NULL, NULL, NULL, QUERY_CONNECTED_STORAGE_DEVICE, NULL, NULL,
		(EVT_SUBSCRIBE_CALLBACK)ConnectedStorageDeviceCallback, EvtSubscribeToFutureEvents);
	if (NULL == hConnectedStorageDeviceSubscription)
	{
		status = GetLastError();

		if (ERROR_EVT_CHANNEL_NOT_FOUND == status)
			wprintf(L"Channel %s was not found.\n", QUERY_CONNECTED_STORAGE_DEVICE);
		else if (ERROR_EVT_INVALID_QUERY == status)
			// You can call EvtGetExtendedStatus to get information as to why the query is not valid.
			wprintf(L"The query \"%s\" is not valid.\n", QUERY_CONNECTED_STORAGE_DEVICE);
		else
			wprintf(L"EvtSubscribe hConnectedStorageDeviceSubscription failed with %lu.\n", status);

		goto cleanup;
	}
	else {
		wprintf(L"hConnectedStorageDeviceSubscription done.\n");
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~ SUBSCRIBING PROCESS OPENED ~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

	hProcessOpenedSubscription = EvtSubscribe(NULL, NULL, NULL, QUERY_PROCESS_OPENED, NULL, NULL,
		(EVT_SUBSCRIBE_CALLBACK)AbusableProcessOpenedCallback, EvtSubscribeToFutureEvents);
	if (NULL == hProcessOpenedSubscription)
	{
		status = GetLastError();

		if (ERROR_EVT_CHANNEL_NOT_FOUND == status)
			wprintf(L"Channel %s was not found.\n", QUERY_PROCESS_OPENED);
		else if (ERROR_EVT_INVALID_QUERY == status)
			// You can call EvtGetExtendedStatus to get information as to why the query is not valid.
			wprintf(L"The query \"%s\" is not valid.\n", QUERY_PROCESS_OPENED);
		else
			wprintf(L"EvtSubscribe hProcessOpenedSubscription failed with %lu.\n", status);

		goto cleanup;
	}
	else {
		wprintf(L"hProcessOpenedSubscription done.\n");
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~ DONE SUBSCRIBING! ~~~~~~~~~~~~~~~~~~~~~~~~~~~ //


	switch (action)
	{
		// You should only get the EvtSubscribeActionError action if your subscription flags 
		// includes EvtSubscribeStrict and the channel contains missing event records.
	case EvtSubscribeActionError:
		if (ERROR_EVT_QUERY_RESULT_STALE == (DWORD)hEvent)
		{
			wprintf(L"The subscription callback was notified that event records are missing.\n");
			// Handle if this is an issue for your application.
		}
		else
		{
			wprintf(L"The subscription callback received the following Win32 error: %lu\n", (DWORD)hEvent);
		}
		break;

	case EvtSubscribeActionDeliver:

		systemState.is_locked = true;
		wprintf(L"\n\n\nREACHED LOCKED ACTION! %d\n\n", systemState.is_locked);


		char tmBuff[30];
		ctime_s(tmBuff, sizeof(tmBuff), &legacyStart);
		std::cout << "\n\n\nLOCKED Current time is: " << tmBuff << '\n';

		if (ERROR_SUCCESS != (status = GetEventInfoString(hEvent, &pRenderedContent)))
		{
			goto cleanup;
		}

		break;



	default:
		wprintf(L"SubscriptionCallback: Unknown action.\n");
	}

cleanup:

	if (pRenderedContent)
		free(pRenderedContent);

	if (ERROR_SUCCESS != status)
	{
		// End subscription - Use some kind of IPC mechanism to signal
		// your application to close the subscription handle.

		//TODO: maybe we need to close event also when success?
		if (hUnlockSubscription)
			EvtClose(hUnlockSubscription);
		if (hConnectedWifiSubscription)
			EvtClose(hConnectedWifiSubscription);
		if (hConnectedStorageDeviceSubscription)
			EvtClose(hConnectedStorageDeviceSubscription);
	}

	return status; // The service ignores the returned status.
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~ CALLBACKS ~~~~~~~~~~~~~~~~~~~~~~~~~~~ //



// The callback that receives the events that match the query criteria. 
DWORD WINAPI UnLockCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent)
{
	auto start = std::chrono::system_clock::now();
	auto legacyStart = std::chrono::system_clock::to_time_t(start);
	LPWSTR pRenderedContent = nullptr;

	UNREFERENCED_PARAMETER(pContext);

	DWORD status = ERROR_SUCCESS;

	switch (action)
	{
		// You should only get the EvtSubscribeActionError action if your subscription flags 
		// includes EvtSubscribeStrict and the channel contains missing event records.
	case EvtSubscribeActionError:
		if (ERROR_EVT_QUERY_RESULT_STALE == (DWORD)hEvent)
		{
			wprintf(L"The subscription callback was notified that event records are missing.\n");
			// Handle if this is an issue for your application.
		}
		else
		{
			wprintf(L"The subscription callback received the following Win32 error: %lu\n", (DWORD)hEvent);
		}
		break;

	case EvtSubscribeActionDeliver:

		systemState.is_locked = false;
		wprintf(L"\n\n\nREACHED UNLOCKED ACTION! %d\n\n", systemState.is_locked);

		char tmBuff[30];
		ctime_s(tmBuff, sizeof(tmBuff), &legacyStart);
		std::cout << "\n\n\nUNLOCKED Current time is: " << tmBuff << '\n';

		if (ERROR_SUCCESS != (status = GetEventInfoString(hEvent, &pRenderedContent)))
		{
			goto cleanup;
		}
		break;

	default:
		wprintf(L"SubscriptionCallback: Unknown action.\n");
	}

cleanup:

	if (pRenderedContent)
		free(pRenderedContent);

	if (ERROR_SUCCESS != status)
	{
		// End subscription - Use some kind of IPC mechanism to signal
		// your application to close the subscription handle.
	}

	return status; // The service ignores the returned status.
}


// The callback that receives the events that match the query criteria. 
DWORD WINAPI ConnectedWifiCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent)
{
	auto start = std::chrono::system_clock::now();
	auto legacyStart = std::chrono::system_clock::to_time_t(start);
	LPWSTR pRenderedContent = nullptr;

	UNREFERENCED_PARAMETER(pContext);

	DWORD status = ERROR_SUCCESS;

	switch (action)
	{
		// You should only get the EvtSubscribeActionError action if your subscription flags 
		// includes EvtSubscribeStrict and the channel contains missing event records.
	case EvtSubscribeActionError:
		if (ERROR_EVT_QUERY_RESULT_STALE == (DWORD)hEvent)
		{
			wprintf(L"The ConnectedWifiCallback was notified that event records are missing.\n");
			// Handle if this is an issue for your application.
		}
		else
		{
			wprintf(L"The ConnectedWifiCallback received the following Win32 error: %lu\n", (DWORD)hEvent);
		}
		break;

	case EvtSubscribeActionDeliver:

		wprintf(L"\n\n\nREACHED CONNECTED WIFI ACTION! \n\n");

		char tmBuff[30];
		ctime_s(tmBuff, sizeof(tmBuff), &legacyStart);
		std::cout << "\n\n\CONNECTED WIFI Current time is: " << tmBuff << '\n';

		if (ERROR_SUCCESS != (status = GetEventInfoString(hEvent, &pRenderedContent)))
		{
			goto cleanup;
		}
		
		// Currently not checking wifi history - every wifi is unknown
		systemState.unknown_wifi = true;

		// Printing Event
		wprintf(L"%s\n\n", pRenderedContent);

		systemState.CheckSystemState();

		break;

	default:
		wprintf(L"ConnectedWifiCallback: Unknown action.\n");
	}

cleanup:

	if (pRenderedContent)
		free(pRenderedContent);

	if (ERROR_SUCCESS != status)
	{
		// End subscription - Use some kind of IPC mechanism to signal
		// your application to close the subscription handle.
	}

	return status; // The service ignores the returned status.
}


// The callback that receives the events that match the query criteria. 
DWORD WINAPI ConnectedStorageDeviceCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent)
{
	auto start = std::chrono::system_clock::now();
	auto legacyStart = std::chrono::system_clock::to_time_t(start);
	LPWSTR pRenderedContent = nullptr;



	UNREFERENCED_PARAMETER(pContext);

	DWORD status = ERROR_SUCCESS;

	ifstream USBLogFile;
	tinyxml2::XMLDocument doc;
	string str, SDInstanceID, SDID, usbHistoryStr;

	// open file into stream in binary mode to bypass ctrl+z (26) issue between subkeys
	ifstream depthStream(LPCWSTRToSTR(lpUsbListPathW), ios_base::in | ios_base::binary);
	if (!depthStream.is_open())
	{
		std::cout << "ConnectedStorageDeviceCallback - Exception while opening usb log file\n" << lpUsbListPathW << '\n';
		goto cleanup;
	}
	usbHistoryStr = std::string((std::istreambuf_iterator<char>(depthStream)), std::istreambuf_iterator<char>());

	// lowercase for file - not time efficient! later we can improve this greatly
	for (int i = 0; i < usbHistoryStr.length(); i++) {
		usbHistoryStr[i] = tolower(usbHistoryStr[i]);
	}

	// checking event action value

	switch (action)
	{
	case EvtSubscribeActionError:
		if (ERROR_EVT_QUERY_RESULT_STALE == (DWORD)hEvent)
		{
			wprintf(L"The ConnectedStorageDeviceCallback was notified that event records are missing.\n");
			// Handle if this is an issue for your application.
		}
		else
		{
			wprintf(L"The ConnectedStorageDeviceCallback received the following Win32 error: %lu\n", (DWORD)hEvent);
		}
		break;

	case EvtSubscribeActionDeliver:

		wprintf(L"\n\n\nREACHED CONNECTED STORAGE DEVICE ACTION! \n\n");

		char tmBuff[30];
		ctime_s(tmBuff, sizeof(tmBuff), &legacyStart);
		std::cout << "\n\n\CONNECTED STORAGE DEVICE Current time is: " << tmBuff << '\n';


		// Rendering event

		if (ERROR_SUCCESS != (status = GetEventInfoString(hEvent, &pRenderedContent)))
		{
			goto cleanup;
		}

		str = LPCWSTRToSTR(pRenderedContent);
		if (XML_SUCCESS != doc.Parse(str.c_str())) {
			printf("ConnectedStorageDeviceCallback Error -  Parsing XML from string failed");
			goto cleanup;
		}
		else {
			// parsing event result
			SDInstanceID = doc.FirstChildElement("Event")->FirstChildElement("UserData")\
				->FirstChildElement("UMDFHostDeviceArrivalBegin")\
				->LastChildElement("InstanceId")->GetText();
			// 
			SDID = parseInstanceId(SDInstanceID);
		}


		// Check if the new connected usb is on the history list
		for (int i = 0; i < SDID.length(); i++) {
			SDID[i] = tolower(SDID[i]);
		}

		if ((usbHistoryStr.find(SDID) == string::npos) || (systemState.usb_debug_mode == true)) {
			// update system state
			systemState.unknown_usb = true;
			printf("ALERT USB UNRECOGNIZED\n");
		}
		else {
			printf("Known Usb storage device connected\n");
		}


		// Printing Event
		wprintf(L"%s\n\n", pRenderedContent);


		systemState.CheckSystemState();

		break;

	default:
		wprintf(L"ConnectedStorageDeviceCallback: Unknown action.\n");
	}

cleanup:

	if (pRenderedContent)
		free(pRenderedContent);

	if (ERROR_SUCCESS != status)
	{
		// End subscription - Use some kind of IPC mechanism to signal
		// your application to close the subscription handle.
	}

	return status; // The service ignores the returned status.
}



// The callback that receives the events that match the query criteria. 
DWORD WINAPI AbusableProcessOpenedCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent)
{
	auto start = std::chrono::system_clock::now();
	auto legacyStart = std::chrono::system_clock::to_time_t(start);
	LPWSTR pRenderedContent = nullptr;

	UNREFERENCED_PARAMETER(pContext);

	DWORD status = ERROR_SUCCESS;

	switch (action)
	{
		// You should only get the EvtSubscribeActionError action if your subscription flags 
		// includes EvtSubscribeStrict and the channel contains missing event records.
	case EvtSubscribeActionError:
		if (ERROR_EVT_QUERY_RESULT_STALE == (DWORD)hEvent)
		{
			wprintf(L"The AbusableProcessOpenedCallback was notified that event records are missing.\n");
			// Handle if this is an issue for your application.
		}
		else
		{
			wprintf(L"The AbusableProcessOpenedCallback received the following Win32 error: %lu\n", (DWORD)hEvent);
		}
		break;

	case EvtSubscribeActionDeliver:

		wprintf(L"\n\n\nREACHED PROCESS ACTION! \n\n");

		char tmBuff[30];
		ctime_s(tmBuff, sizeof(tmBuff), &legacyStart);
		std::cout << "\n\n\PROCESS Current time is: " << tmBuff << '\n';

		// Rendering event
		if (ERROR_SUCCESS != (status = GetEventInfoString(hEvent, &pRenderedContent)))
		{
			goto cleanup;
		}

		// update system state:
		systemState.abusable_process = true;

		std::cout << "\n\n\PROCESS is great SUCCESS: \n" << tmBuff << '\n';

		// Printing Event

		wprintf(L"%s\n\n", pRenderedContent);

		systemState.CheckSystemState();

		break;

	default:
		wprintf(L"AbusableProcessOpenedCallback: Unknown action.\n");
	}

cleanup:

	if (pRenderedContent)
		free(pRenderedContent);

	if (ERROR_SUCCESS != status)
	{
		// End subscription - Use some kind of IPC mechanism to signal
		// your application to close the subscription handle.
	}

	return status; // The service ignores the returned status.
}






