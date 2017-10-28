#include "RemoteImage.h"
#include <algorithm>


RemoteImage::RemoteImage()
{
	remProcess = new RemoteProcess();
	knownModuleBases = new vector<MAPPED_DLL>();
}


RemoteImage::~RemoteImage()
{
	delete remProcess;
	delete knownModuleBases;
}

void RemoteImage::OpenRemoteProcess(string remoteProcess)
{
	remProcess->OpenRemoteProcess(remoteProcess);
}

unique_ptr<Util::ManagedBuffer> RemoteImage::GetDosHeader(string remoteModule, void** remoteBase)
{
	int bufferSize = 0;
	unique_ptr<Util::ManagedBuffer> ret;
	void* dosHeaderBase = nullptr;

	ret = make_unique<Util::ManagedBuffer>();
	dosHeaderBase = GetRemoteModuleBase(remoteModule);
	//ret->SetContent(this->remProcess->ReadBuffer())
	return ret;
}

unique_ptr<Util::ManagedBuffer> RemoteImage::GetNtHeader(string remoteModule, void** remoteBase)
{
	return nullptr;
}

void* RemoteImage::GetRemoteModuleBase(string moduleName)
{

	//first check the already known addresses
	for(vector<MAPPED_DLL>::iterator it = knownModuleBases->begin(); it != knownModuleBases->end();it++)
	{
		if(it->moduleName == moduleName)
		{
			return it->remoteBase;
		}
	}

	int remoteProcessPid = -1;

	remoteProcessPid = this->remProcess->GetRemotePid();

	if (remoteProcessPid == -1)
	{
		return nullptr;
	}

	HANDLE enumHandle = nullptr;
	MODULEENTRY32 moduleInfoStorage = { 0 };
	string compareString;

	transform(moduleName.begin(), moduleName.end(), moduleName.begin(), toupper);

	enumHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, remoteProcessPid);
	if (enumHandle == INVALID_HANDLE_VALUE)
	{
		return (void*)-1;
	}

	moduleInfoStorage.dwSize = sizeof(moduleInfoStorage);
	if (Module32First(enumHandle, &moduleInfoStorage) == FALSE)
	{
		CloseHandle(enumHandle);
		return (void*)-1;
	}
	//is it the first one?
	compareString = string(moduleInfoStorage.szModule);
	transform(compareString.begin(), compareString.end(), compareString.begin(), toupper);
	if (compareString == moduleName)
	{
		CloseHandle(enumHandle);
		return moduleInfoStorage.modBaseAddr;
	}

	//loop throug the remaining modules
	do
	{
		moduleInfoStorage.dwSize = sizeof(moduleInfoStorage);
		compareString = string(moduleInfoStorage.szModule);
		transform(compareString.begin(), compareString.end(), compareString.begin(), toupper);
		if (compareString == moduleName)
		{
			break;
		}
	} while (Module32Next(enumHandle, &moduleInfoStorage) == TRUE);

	CloseHandle(enumHandle);
	//check if the ModuleNext gave us the NOMOREMODULE error
	if(GetLastError() == ERROR_NO_MORE_FILES)
	{
		return reinterpret_cast<void*>(-1);
	}
	return moduleInfoStorage.modBaseAddr;
}
