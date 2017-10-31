#include "ProcAddressExtractor.h"

//i know its not nice, cause the validity of this object depends on the ref from outside to be valid unique_ptr and such things...
ProcAddressExtractor::ProcAddressExtractor(RemoteImage* importFromProcess)
{
	this->importProcess = importFromProcess;
}

ProcAddressExtractor::~ProcAddressExtractor()
{
}

//returns the 
BITDYNAMIC ProcAddressExtractor::GetProcAddress(string moduleName, string funcName)
{
	//check if the desired image is already loaded
	if(importProcess->GetRemoteModuleBase(moduleName) == reinterpret_cast<void*>(-1))
	{
		//recursiv loading of images
		DllSearcher* tempSearcher = new DllSearcher();
		auto diskDll = 
	}
	//load the new export directory, if we want to import from a different module than last time
	if(moduleName != this->currentImportModule)
	{
		this->currentRemoteExportDir = this->importProcess->GetDataDirectoryContentByIndex(modulename)
	}
	return 0;
}

BITDYNAMIC ProcAddressExtractor::GetProcAddress(string moduleName, int ordinal)
{
	return 0;
}
