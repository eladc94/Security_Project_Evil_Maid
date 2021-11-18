#include "Utils.h"
#include <vector>
using namespace std;
using namespace tinyxml2;


// Render the event as an XML string and print it.
DWORD GetEventInfoString(EVT_HANDLE hEvent, LPWSTR* evtContent)
{
	DWORD status = ERROR_SUCCESS;
	DWORD dwBufferSize = 0;
	DWORD dwBufferUsed = 0;
	DWORD dwPropertyCount = 0;
	LPWSTR pRenderedContent = NULL;


	if (!EvtRender(NULL, hEvent, EvtRenderEventXml, dwBufferSize, pRenderedContent, &dwBufferUsed, &dwPropertyCount))
	{
		if (ERROR_INSUFFICIENT_BUFFER == (status = GetLastError()))
		{
			dwBufferSize = dwBufferUsed;
			pRenderedContent = (LPWSTR)malloc(dwBufferSize);
			if (pRenderedContent)
			{
				EvtRender(NULL, hEvent, EvtRenderEventXml, dwBufferSize, pRenderedContent, &dwBufferUsed, &dwPropertyCount);
				*evtContent = pRenderedContent;
			}
			else
			{
				wprintf(L"malloc failed\n");
				status = ERROR_OUTOFMEMORY;
				goto cleanup;
			}
		}

		if (ERROR_SUCCESS != (status = GetLastError()))
		{
			wprintf(L"EvtRender failed with %d\n", status);
			goto cleanup;
		}
	}

cleanup:

	// free for content buffer is happening on the caller side

	return status;
}

DWORD WINAPI DumpPreviousUSBList(LPCWSTR& lpUsbListPathW)
{

	HANDLE ProcessToken;

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &ProcessToken)) {

		SetPrivilege(ProcessToken, SE_BACKUP_NAME, TRUE);

		string lpUsbListPath = LPCWSTRToSTR(lpUsbListPathW);

		// if the file with the name exists - remove it
		ifstream USBLogFile;
		try
		{
			USBLogFile.open(lpUsbListPath);
			if (USBLogFile) {
				USBLogFile.close();
				if (remove(lpUsbListPath.c_str()) != 0)
					perror("Error deleting USB history file\n");
				else
					printf("USB history File successfully deleted\n");
			}
			else {
				// previous file does not exist, no need to delete it
				USBLogFile.close();
			}
		}
		catch (const std::exception&)
		{
			printf("Exception while opening usb log file\n");
			return ERROR_FILE_NOT_FOUND;
		}

		// Writing to the usb log file from the registry key

		LONG lResult;
		HKEY hKey;
		LPCTSTR lpSubKey = L"SYSTEM\\CurrentControlSet\\Enum\\USBSTOR";

		lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpSubKey, 0, KEY_READ, &hKey);

		if (lResult != ERROR_SUCCESS)
		{
			if (lResult == ERROR_FILE_NOT_FOUND) {
				printf("Key not found.\n");
				return lResult;
			}
			else {
				printf("Error opening key.\n");
				return lResult;
			}
		}

		lResult = RegSaveKey(hKey, lpUsbListPathW, nullptr);

		if (lResult != ERROR_SUCCESS)
		{
			if (lResult == ERROR_FILE_NOT_FOUND) {
				printf("LogPath not found.\n");
				return lResult;
			}
			if (lResult == ERROR_ALREADY_EXISTS) {
				printf("LogPath points to an already existing file.\n");
				return lResult;
			}
			else {
				printf("Error opening key.\n");
				return lResult;
			}
		}
		else {
			printf("USB history File successfully created\n");
		}

		RegCloseKey(hKey);

		return lResult;
	}
}

/// <summary>
/// This function is meant to "clean" the usb instance ID from a string given to it
/// </summary>
/// <param name="fullId"></param>
/// <returns></returns>

string parseInstanceId(string& fullId) {
	vector<string> idvector, idinnervector;
	stringstream sstream(fullId);
	string return_val;
	while (std::getline(sstream, return_val, '#'))
	{
		idvector.push_back(return_val);
	}

	return_val = *(++idvector.rbegin());

	// get rid of the &0 or &1 in the end
	return_val.pop_back();
	return_val.pop_back();

	return return_val;
}

string LPCWSTRToSTR(LPCWSTR ptr) {
	wstring ws(ptr);
	string str(ws.begin(), ws.end());
	return str;
}


/// <summary>
/// Send email to notify user
/// </summary>
void SendEmail(unsigned int state) {

	const char* command = nullptr;

	if (state == 1)
	{
		command = "curl smtp://smtp.gmail.com:587 -v --mail-from \"TechnionSecProjB2021@gmail.com\" --mail-rcpt \"TechnionSecProjB2021@gmail.com\" --ssl -u TechnionSecProjB2021@gmail.com:PASSWORD_HERE -T \"Minor_Notification.txt\" -k --anyauth";
	}
	if (state == 2)
	{
		command = "curl smtp://smtp.gmail.com:587 -v --mail-from \"TechnionSecProjB2021@gmail.com\" --mail-rcpt \"TechnionSecProjB2021@gmail.com\" --ssl -u TechnionSecProjB2021@gmail.com:PASSWORD_HERE -T \"Major_Notification.txt\" -k --anyauth";
	}
	WinExec(command, SW_HIDE);

}
