#include "RemoteProcess.h"


RemoteProcess::RemoteProcess()
{
	this->processHandle = INVALID_HANDLE_VALUE;
	this->processName = "";
	this->remotePid = -1;
}

RemoteProcess::~RemoteProcess()
{
	if (this->processHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(this->processHandle);
	}
}

void RemoteProcess::CheckOpened()
{
	if (!this->processOpened)
	{
		throw string("Process not opened. Cant interact with process");
	}
}

int RemoteProcess::GetRemotePid()
{
	return remotePid;
}

void RemoteProcess::OpenRemoteProcess(string processName)
{
	int tempProcPid = -1;
	HANDLE tempHandle = INVALID_HANDLE_VALUE;

	if (processName.empty())
	{
		throw string("Given process name string empty");
	}
	tempProcPid = GetPidFromName(processName);
	if (tempProcPid == -1)
	{
		throw string("No process found with given name");
	}
	tempHandle = OpenProcess(PROCESS_ALL_ACCESS, false, tempProcPid);
	if (tempHandle == nullptr)
	{
		string ret("Could not open process with ALL_ACCESS. GetLastError():");
		ret += to_string(GetLastError());
		throw ret;
	}
	this->processHandle = tempHandle;
	this->processName = processName;
	this->remotePid = tempProcPid;
	this->processOpened = true;
}

#pragma region Read Values
int RemoteProcess::ReadInt(void* readAddress)
{
	CheckOpened();
	int ret = 0;
	byte* readBuffer = nullptr;
	readBuffer = ReadBuffer(readAddress, sizeof(int));
	ret = *(reinterpret_cast<int*>(readBuffer));
	free(readBuffer);
	return ret;
}

DWORD RemoteProcess::ReadDword(void* readAddress)
{
	CheckOpened();
	DWORD ret = 0;
	byte* readBuffer = nullptr;
	readBuffer = ReadBuffer(readAddress, sizeof(DWORD));
	ret = *(reinterpret_cast<DWORD*>(readBuffer));
	free(readBuffer);
	return ret;
}

long long RemoteProcess::ReadLongLong(void* readAddress)
{
	CheckOpened();
	long long ret = 0;
	byte* readBuffer = nullptr;
	readBuffer = ReadBuffer(readAddress, sizeof(long long));
	ret = *(reinterpret_cast<long long*>(readBuffer));
	free(readBuffer);
	return ret;
}

string RemoteProcess::ReadAnsiString(void * readAddress, int maxLenght)
{
	CheckOpened();
	string ret;
	char tempByte = NULL;
	int limitCounter = 0;
	char* readPointer = nullptr;
	if(maxLenght == 0)
	{
		throw string("Tried to read 0 lenght remote string");
	}
	readPointer = reinterpret_cast<char*>(readAddress);

	while(limitCounter <= maxLenght)
	{
		if (!ReadProcessMemory(this->processHandle, readPointer, &tempByte, sizeof(char),nullptr))
		{
			throw string("Could not read remote string");
		}

		if (tempByte == 0)
			break;
		ret += tempByte;
		limitCounter++;
		readPointer++;
	}
	if(limitCounter > maxLenght)
	{
		throw string("Maximum exceeded while reading remote string");
	}
	return ret;
}

byte* RemoteProcess::ReadBuffer(void* readAddress, int bufferSize)
{
	CheckOpened();
	byte* ret = nullptr;
	SIZE_T bytesRead = 0;

	ret = reinterpret_cast<byte*>(calloc(bufferSize, sizeof(byte)));
	if (!ReadProcessMemory(this->processHandle, readAddress, ret, bufferSize, &bytesRead))
	{
		free(ret);
		throw string("Could not read remote process memory");
	}
	if (bytesRead != bufferSize)
	{
		free(ret);
		string retString("Could not read all of the desired data");
		retString += to_string(GetLastError());
		throw retString;
	}
	return ret;
}
#pragma endregion

#pragma region Write Values
void RemoteProcess::WriteInt(void* writeAddress, int value)
{
	CheckOpened();
	WriteBuffer(writeAddress, &value, sizeof(int));
}

void RemoteProcess::WriteDword(void* writeAddress, DWORD value)
{
	CheckOpened();
	WriteBuffer(writeAddress, &value, sizeof(DWORD));
}

void RemoteProcess::WriteLongLong(void* writeAddress, long long value)
{
	CheckOpened();
	WriteBuffer(writeAddress, &value, sizeof(long long));
}

void RemoteProcess::WriteAnsiString(void* writeAddress, string value)
{
	CheckOpened();
	//what is this?! why must these cast be so crazy if wnat to stay in c++ formality
	WriteBuffer(writeAddress, const_cast<void*>(reinterpret_cast<const void*>(value.c_str())), static_cast<int>(value.length()));
}

void RemoteProcess::WriteBuffer(void* writeAddress, void* buffer, int bufferSize)
{
	CheckOpened();
	if(WriteProcessMemory(processHandle, writeAddress, buffer, bufferSize, nullptr) == 0)
	{
		throw string("Could not write the (complete) Integer into remote Process");
	}
}

#pragma endregion

#pragma region Allocation

void* RemoteProcess::AllocSection(int sectionSize, DWORD sectionPermission)
{
	CheckOpened();
	void* ret = nullptr;
	if((ret = VirtualAllocEx(processHandle,nullptr,sectionSize,MEM_COMMIT|MEM_RESERVE,sectionPermission)) == nullptr)
	{
		throw string("Error allocating section in remoteProcess");
	}
	return ret;
}

#pragma endregion

//static
int RemoteProcess::GetPidFromName(string processName)
{
	HANDLE enumHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32 currentProcess = {0};
	currentProcess.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(enumHandle, &currentProcess))
	{
		return -1;
	}

	while (string(currentProcess.szExeFile) != processName)
	{
		if (!Process32Next(enumHandle, &currentProcess))
			return -1;
	}
	return currentProcess.th32ProcessID;
}
