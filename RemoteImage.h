#pragma once

#include <Windows.h>
#include <memory>
#include "Util.h"

#include "RemoteProcess.h"
#include "PeHelper.h"
#include <vector>
#include "DllOnDisk.h"
using namespace std;

typedef struct _MAPPED_DLL
{
	string moduleName;
	void* remoteBase;
}MAPPED_DLL;


class RemoteImage
{
	RemoteProcess* remProcess;
	vector<MAPPED_DLL>* knownModuleBases;
public:
	RemoteImage();
	~RemoteImage();

	void OpenRemoteProcess(string remoteProcess);

	unique_ptr<Util::ManagedBuffer<IMAGE_DOS_HEADER*>> GetDosHeader(string remoteModule, void** remoteBase);
	unique_ptr<Util::ManagedBuffer<IMAGE_NT_HEADERS*>> GetNtHeader(string remoteModule, void** remoteBase);

	unique_ptr <Util::ManagedBuffer<IMAGE_SECTION_HEADER*>> GetSectionHeaderByIndex(string remoteModule, int index, void** remoteBase);
	unique_ptr <Util::ManagedBuffer<void*>> GetSectionContentByIndex(string remoteModule, int index, void** remoteBase);

	unique_ptr <Util::ManagedBuffer<void*>> GetDataDirectoryContentByIndex(string remoteModule, unsigned int index, void** remoteBase);

	RemoteProcess* GetRemProcInstance();
	void* GetRemoteModuleBase(string moduleName);
	void AddModuleBaseMapping(void* remoteAddress, string remoteModuleName);

	void* AllocSpaceForDll(DllOnDisk*& dll);
	void WriteDllToSection(DllOnDisk*& preparedDll, void* remoteSectionBase);
};

