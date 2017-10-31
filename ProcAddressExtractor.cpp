#include "ProcAddressExtractor.h"
#include <iostream>

//i know its not nice, cause the validity of this object depends on the ref from outside to be valid unique_ptr and such things...
ProcAddressExtractor::ProcAddressExtractor(RemoteImage* importFromProcess)
{
	this->importProcess = importFromProcess;
}

ProcAddressExtractor::~ProcAddressExtractor()
{
}

//returns the 
BITDYNAMIC* ProcAddressExtractor::GetProcAddress(string moduleName, string funcName)
{
	//check if the desired image is already loaded
	if((currentModuleBase = reinterpret_cast<int>(importProcess->GetRemoteModuleBase(moduleName))) == -1)
	{
		//cannot get address as module is not loaded
		return 0;
	}
	//load the new export directory, if we want to import from a different module than last time
	if(moduleName != this->currentImportModule)
	{
		this->currentImportModule = moduleName;
		this->currentRemoteExportDir = this->importProcess->GetDataDirectoryContentByIndex(moduleName,
																							IMAGE_DIRECTORY_ENTRY_EXPORT,
																							reinterpret_cast<void**>(&this->currentExportDirBase));
	}

	//search for name to import
	BITDYNAMIC* funcAddress = 0;
	IMAGE_EXPORT_DIRECTORY* exportDir = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(currentRemoteExportDir->GetContent());
	DWORD* nameIterator = reinterpret_cast<DWORD*>(0xFFFFFFFF);
	int offsetToRemoteExportDir = (currentExportDirBase - currentModuleBase);
	int nameOffset = 0;
	//check if the desired address is inside of the already read remoteExport directory
	if(exportDir->AddressOfNames - offsetToRemoteExportDir < currentRemoteExportDir->GetBufferSize())
	{
		nameIterator = RESOLVE_RVA(DWORD*, currentRemoteExportDir->GetContent(), exportDir->AddressOfNames - offsetToRemoteExportDir);
	}
	else
	{
		cout << "FORWARDED EXPORT DETECTED!!!!" << endl;
	}

	for(int i = 0; i < exportDir->NumberOfNames;i++)
	{
		string currentExportFuncName = string(RESOLVE_RVA(char*, currentRemoteExportDir->GetContent(), (nameIterator[i]) - offsetToRemoteExportDir));
		if(currentExportFuncName == funcName)
		{
			WORD* ordinalBase = RESOLVE_RVA(WORD*, currentRemoteExportDir->GetContent(), exportDir->AddressOfNameOrdinals - offsetToRemoteExportDir);
			//first get the correct ordinal entry
			DWORD ordinalIndex = ordinalBase[i];
			DWORD* remoteFuncAddress = RESOLVE_RVA(DWORD*, currentRemoteExportDir->GetContent(), exportDir->AddressOfFunctions - offsetToRemoteExportDir);
			funcAddress = RESOLVE_RVA(BITDYNAMIC*, currentModuleBase, remoteFuncAddress[ordinalIndex]);
		}
	}
	return funcAddress;
}

BITDYNAMIC* ProcAddressExtractor::GetProcAddress(string moduleName, int ordinal)
{
	//check if the desired image is already loaded
	if ((currentModuleBase = reinterpret_cast<int>(importProcess->GetRemoteModuleBase(moduleName))) == -1)
	{
		//cannot get address as module is not loaded
		return nullptr;
	}
	//load the new export directory, if we want to import from a different module than last time
	if (moduleName != this->currentImportModule)
	{
		this->currentImportModule = moduleName;
		this->currentRemoteExportDir = this->importProcess->GetDataDirectoryContentByIndex(moduleName,
			IMAGE_DIRECTORY_ENTRY_EXPORT,
			reinterpret_cast<void**>(&this->currentExportDirBase));
	}

	BITDYNAMIC* returnAddress = nullptr;
	int offsetToRemoteExportDir = (currentExportDirBase - currentModuleBase);
	IMAGE_EXPORT_DIRECTORY* exportDir = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(currentRemoteExportDir->GetContent());
	BITDYNAMIC* functionAddressBase = RESOLVE_RVA(BITDYNAMIC*, currentRemoteExportDir->GetContent(), exportDir->AddressOfFunctions - offsetToRemoteExportDir);

	returnAddress = RESOLVE_RVA(BITDYNAMIC*, currentRemoteExportDir->GetContent(), functionAddressBase[ordinal + exportDir->Base]);

	return returnAddress;
}
