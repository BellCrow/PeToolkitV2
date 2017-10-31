#pragma once
#include "Util.h"
#include "RemoteImage.h"
#include "DllSearcher.h"
#include "ManualInjector.h"
#include <Windows.h>
#include <memory>
#include <string>

using namespace std;
class ProcAddressExtractor
{
	RemoteImage* importProcess;
	string currentImportModule;
	BITDYNAMIC currentModuleBase;
	BITDYNAMIC currentImageBase;
	UNIQUE_MANAGED(void*) currentRemoteExportDir;
public:
	ProcAddressExtractor(RemoteImage* importFromProcess);
	~ProcAddressExtractor();

	BITDYNAMIC GetProcAddress(string moduleName, string funcName);
	BITDYNAMIC GetProcAddress(string moduleName, int ordinal);
};

