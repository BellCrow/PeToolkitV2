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

	int ReadInt(void* readAddress);
	DWORD ReadDword(void* readAddress);
	long long ReadLongLong(void* readAddress);
	string ReadAnsiString(void* readAddress,int maxLenght);
	byte* ReadBuffer(void* readAddress, int bufferSize);
	

	void WriteInt(void* writeAddress, int value);
	void WriteDword(void* writeAddress, DWORD value);
	void WriteLongLong(void* writeAddress, long long value);
	void WriteAnsiString(void* writeAddress, string value);
	void WriteBuffer(void* writeAddress, void* buffer, int bufferSize);


	static int GetPidFromName(string processName);
};
