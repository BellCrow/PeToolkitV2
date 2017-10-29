#pragma once

#include <Windows.h>
#include <string>
#include <TlHelp32.h>

using namespace std;

class RemoteProcess
{

	HANDLE processHandle;
	string processName;
	int remotePid;
	bool processOpened;

	void CheckOpened();
public:
	RemoteProcess();
	~RemoteProcess();

	//getter

	int GetRemotePid();

	void OpenRemoteProcess(string processName);

	#pragma region Read values

	int ReadInt(void* readAddress);
	DWORD ReadDword(void* readAddress);
	long long ReadLongLong(void* readAddress);
	string ReadAnsiString(void* readAddress,int maxLenght);
	byte* ReadBuffer(void* readAddress, int bufferSize);

	#pragma endregion

	#pragma region Write Values

	void WriteInt(void* writeAddress, int value);
	void WriteDword(void* writeAddress, DWORD value);
	void WriteLongLong(void* writeAddress, long long value);
	void WriteAnsiString(void* writeAddress, string value);
	void WriteBuffer(void* writeAddress, void* buffer, int bufferSize);

	#pragma endregion
	
	#pragma region Allocation	
	void* AllocSection(int sectionSize, DWORD sectionPermission);
	#pragma endregion

	static int GetPidFromName(string processName);
};
