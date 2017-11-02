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
	if((currentModuleBase = reinterpret_cast<BITDYNAMIC>(importProcess->GetRemoteModuleBase(moduleName))) == -1)
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

	IMAGE_EXPORT_DIRECTORY* exportDir = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(currentRemoteExportDir->GetContent());
	//search for name to import
	DWORD* remoteFuncNameBase = RESOLVE_RVA(DWORD*,currentModuleBase,exportDir->AddressOfNames);
	WORD* remoteOrdinalBase = RESOLVE_RVA(WORD*, currentModuleBase, exportDir->AddressOfNameOrdinals);
	DWORD* remoteFuncRvaBase = RESOLVE_RVA(DWORD*, currentModuleBase, exportDir->AddressOfFunctions);


	string exportNameIterator;
	WORD funcOrdinal = 0;
	DWORD remoteFuncRva = 0;

	int i = 0;
	for(i = 0; i< exportDir->AddressOfNames;i++)
	{
		exportNameIterator = importProcess->GetRemProcInstance()->ReadAnsiString(RESOLVE_RVA(void*,currentModuleBase, importProcess->GetRemProcInstance()->ReadDword(remoteFuncNameBase + i)), 60);
		if (exportNameIterator == funcName)
		{
			break;
		}
	}
	funcOrdinal = importProcess->GetRemProcInstance()->ReadWord(remoteOrdinalBase + i);
	remoteFuncRva = importProcess->GetRemProcInstance()->ReadDword(remoteFuncRvaBase + funcOrdinal);
	//check for a forwarded export
	if(RESOLVE_RVA(BITDYNAMIC, currentModuleBase, remoteFuncRva) > currentExportDirBase
		&& RESOLVE_RVA(void*, currentModuleBase, remoteFuncRva) < RESOLVE_RVA(void*, currentExportDirBase, currentRemoteExportDir->GetBufferSize()))
	{
		cout << "Forwarded export found " << exportNameIterator << endl;

		exportNameIterator = importProcess->GetRemProcInstance()->ReadAnsiString(RESOLVE_RVA(void*, currentModuleBase, remoteFuncRva), 60);
		string dllnameArg = "";
		string FuncNameArg = "";
		dllnameArg = exportNameIterator.substr(0, exportNameIterator.find('.')) + ".dll";
		FuncNameArg = exportNameIterator.substr(exportNameIterator.find('.') + 1, string::npos);
		return GetProcAddress(dllnameArg, FuncNameArg);
	}
	return RESOLVE_RVA(BITDYNAMIC*, currentModuleBase, remoteFuncRva);
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
