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

unique_ptr<Util::ManagedBuffer<IMAGE_DOS_HEADER*>> RemoteImage::GetDosHeader(string remoteModule, void** remoteBase)
{
	int bufferSize = 0;
	UNIQUE_MANAGED(IMAGE_DOS_HEADER*) ret;
	void* dosHeaderBase = nullptr;

	ret = make_unique<Util::ManagedBuffer<IMAGE_DOS_HEADER*>>();
	dosHeaderBase = GetRemoteModuleBase(remoteModule);
	ret->SetContent(reinterpret_cast<IMAGE_DOS_HEADER*>(this->remProcess->ReadBuffer(dosHeaderBase, sizeof(IMAGE_DOS_HEADER))));
	ret->SetBufferSize(sizeof(IMAGE_DOS_HEADER));
	if (remoteBase != nullptr)
	{
		*remoteBase = dosHeaderBase;
	}
	return ret;
}

unique_ptr<Util::ManagedBuffer<IMAGE_NT_HEADERS*>> RemoteImage::GetNtHeader(string remoteModule, void** remoteBase)
{
	void* dosBaseAddress = nullptr;
	void* ntBase = nullptr;
	UNIQUE_MANAGED(IMAGE_DOS_HEADER*) dosHeader = GetDosHeader(remoteModule, &dosBaseAddress);
	UNIQUE_MANAGED(IMAGE_NT_HEADERS*) ntHeader = make_unique<Util::ManagedBuffer<IMAGE_NT_HEADERS*>>();
	ntBase = RESOLVE_RVA(void*, dosBaseAddress, dosHeader->GetContent()->e_lfanew);

	ntHeader->SetContent(reinterpret_cast<IMAGE_NT_HEADERS*>(this->remProcess->ReadBuffer(ntBase, sizeof(IMAGE_NT_HEADERS))));
	ntHeader->SetBufferSize(sizeof(IMAGE_SECTION_HEADER));
	if(remoteBase != nullptr)
	{
		*remoteBase = ntBase;
	}
	return ntHeader;
}

unique_ptr<Util::ManagedBuffer<IMAGE_SECTION_HEADER*>> RemoteImage::GetSectionHeaderByIndex(string remoteModule, int index, void** remoteBase)
{
	void* remoteImagebase = nullptr;
	void* remoteNtHeaderBase = nullptr;
	IMAGE_SECTION_HEADER* sectionBaseRemoteAddress = nullptr;
	
	UNIQUE_MANAGED(IMAGE_SECTION_HEADER*) sectionContent = make_unique<Util::ManagedBuffer<IMAGE_SECTION_HEADER*>>();

	UNIQUE_MANAGED(IMAGE_NT_HEADERS*) ntHeader = GetNtHeader(remoteModule,&remoteNtHeaderBase);

	remoteImagebase = GetRemoteModuleBase(remoteModule);
	if(ntHeader->GetContent()->FileHeader.NumberOfSections <= index)
	{
		throw string("Desired section index ist out bounds for remote image");
	}
	
	//get address of the first sectionHeader directly after the nt Header
	sectionBaseRemoteAddress = RESOLVE_RVA(IMAGE_SECTION_HEADER*, remoteNtHeaderBase, sizeof(ntHeader->GetContent()->Signature));
	sectionBaseRemoteAddress = RESOLVE_RVA(IMAGE_SECTION_HEADER*, sectionBaseRemoteAddress, sizeof(ntHeader->GetContent()->FileHeader));
	sectionBaseRemoteAddress = RESOLVE_RVA(IMAGE_SECTION_HEADER*, sectionBaseRemoteAddress, ntHeader->GetContent()->FileHeader.SizeOfOptionalHeader);

	sectionBaseRemoteAddress = reinterpret_cast<IMAGE_SECTION_HEADER*>(&sectionBaseRemoteAddress[index]);
	//read the sectionHeader content
	sectionContent->SetContent(reinterpret_cast<IMAGE_SECTION_HEADER*>(reinterpret_cast<IMAGE_SECTION_HEADER*>(remProcess->ReadBuffer(sectionBaseRemoteAddress, sizeof(IMAGE_SECTION_HEADER)))));
	sectionContent->SetBufferSize(sizeof(IMAGE_SECTION_HEADER));
	if(remoteBase != nullptr)
	{
		*remoteBase = sectionBaseRemoteAddress;
	}
	return sectionContent;
}

unique_ptr<Util::ManagedBuffer<void*>> RemoteImage::GetSectionContentByIndex(string remoteModule, int index, void** remoteBase)
{
	void* imageBase = GetRemoteModuleBase(remoteModule);
	auto sectionHeader = GetSectionHeaderByIndex(remoteModule, index, nullptr);
	UNIQUE_MANAGED(void*) sectionContent = make_unique<Util::ManagedBuffer<void*>>();
	void* sectionBaseAddress = RESOLVE_RVA(void*, imageBase, sectionHeader->GetContent()->VirtualAddress);

	sectionContent->SetContent(remProcess->ReadBuffer(sectionBaseAddress, sectionHeader->GetContent()->Misc.VirtualSize));
	sectionContent->SetBufferSize(sectionHeader->GetContent()->Misc.VirtualSize);
	if(remoteBase != nullptr)
	{
		*remoteBase = sectionBaseAddress;
	}
	return sectionContent;
}

unique_ptr<Util::ManagedBuffer<void*>> RemoteImage::GetDataDirectoryContentByIndex(string remoteModule, unsigned int index, void** remoteBase)
{
	void* remoteModuleBase = GetRemoteModuleBase(remoteModule);
	UNIQUE_MANAGED(IMAGE_NT_HEADERS*) ntHeader = GetNtHeader(remoteModule, nullptr);
	void* ddHeaderAddress = nullptr;
	UNIQUE_MANAGED(void*) ddContent = make_unique<Util::ManagedBuffer<void*>>();
	
	if(ntHeader->GetContent()->OptionalHeader.NumberOfRvaAndSizes <= index)
	{
		throw string("Desired Datadirectory index is out of bounds for the remote image");
	}

	ddHeaderAddress = RESOLVE_RVA(void*, remoteModuleBase, ntHeader->GetContent()->OptionalHeader.DataDirectory[index].VirtualAddress);
	ddContent->SetContent(remProcess->ReadBuffer(ddHeaderAddress, ntHeader->GetContent()->OptionalHeader.DataDirectory[index].Size));
	ddContent->SetBufferSize(ntHeader->GetContent()->OptionalHeader.DataDirectory[index].Size);

	return ddContent;
}

void* RemoteImage::GetRemoteModuleBase(string moduleName)
{
	//tranform the given modulename to upper case 
	//no case sensitive
	transform(moduleName.begin(), moduleName.end(), moduleName.begin(), toupper);

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
	//add the newly needed modulebase to the list of known bases
	MAPPED_DLL adder;
	adder.remoteBase = moduleInfoStorage.modBaseAddr;
	adder.moduleName = moduleName;
	knownModuleBases->push_back(adder);
	return moduleInfoStorage.modBaseAddr;
}

void RemoteImage::AddModuleBaseMapping(void* remoteAddress, string remoteModuleName)
{
	MAPPED_DLL adder;
	adder.remoteBase = remoteAddress;
	adder.moduleName = remoteModuleName;
	knownModuleBases->push_back(adder);
}

void* RemoteImage::AllocSpaceForDll(DllOnDisk*& dll)
{
	return remProcess->AllocSection(dll->GetNtHeader()->OptionalHeader.SizeOfImage, PAGE_EXECUTE_READWRITE);
}
