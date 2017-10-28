#pragma once
#include "RemoteProcess.h"
#include "PeHelper.h"
#include <Windows.h>
#include <memory>
#include "Util.h"
#include <vector>
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

	unique_ptr<Util::ManagedBuffer> GetDosHeader(string remoteModule, void** remoteBase);
	unique_ptr<Util::ManagedBuffer> GetNtHeader(string remoteModule, void** remoteBase);

	void* GetRemoteModuleBase(string moduleName);
};

