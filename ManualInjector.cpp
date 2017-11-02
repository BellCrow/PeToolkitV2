#include "ManualInjector.h"
#include "Util.h"
#include <iostream>
#include "DllSearcher.h"


ManualInjector::ManualInjector(RemoteImage*& processToInjectInto)
{
	remImage = processToInjectInto;
	procEx = new ProcAddressExtractor(remImage);
}


ManualInjector::~ManualInjector()
{
	delete remImage;
}

void ManualInjector::InjectDll(DllOnDisk*& dllToInject)
{

#ifdef _WIN64
	byte bootStrapper[] =
	{ 0x48, 0x89, 0xC8, 0x4C, 0x8B, 0x00, 0x48, 0x8B, 0x50, 0x08, 0x48, 0x8B, 0x48, 0x10, 0x48, 0x83, 0xEC, 0x28, 0xFF, 0x50, 0x18, 0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x48, 0x83, 0xC4, 0x28, 0xC3 };
	int bootStrapSize = sizeof(bootStrapper);
#else
	byte bootStrapper[] =
	{
		0x55				//push ebp 0x55
		, 0x89, 0xE5         //mov ebp,esp
		, 0x60				//pushal
		, 0x8B, 0x45, 0x08	//mov eax,[ebp+4]
		, 0xFF, 0x30	   //push [eax]
		, 0xFF, 0x70, 0x04 //push [eax+4]
		, 0xFF, 0x70, 0x08 //push [eax+8]
		, 0x8B, 0x40, 0xC //mov eax,[eax+c]
		, 0xFF, 0xD0	   //call eax
		, 0x61				//popal
		, 0x89, 0xEC		//mov esp,ebp
		, 0x5D				//pop ebp
		, 0x31, 0xC0		//xor eax,eax
							//, 0xFE, 0xC0		//inc al
		, 0xC3				//ret (will return 1 here)
	};
	int bootStrapSize = sizeof(bootStrapper);
#endif
	void* remoteSectionBase = nullptr;
	BOOTSTRAPPER_DATA bootStrapData = {0};
	//reserve the neccesary space in the remoteImage
	remoteSectionBase = remImage->AllocSpaceForDll(dllToInject);

	ResolveRelocations(dllToInject, remoteSectionBase);
	ResolveImports(dllToInject);

	remImage->WriteDllToSection(dllToInject, remoteSectionBase);

	void* bootStrapSection = remImage->AllocSpaceForDll(dllToInject);

	bootStrapData.fdwReason = DLL_PROCESS_ATTACH;
	bootStrapData.hInstance = reinterpret_cast<BITDYNAMIC>(remoteSectionBase);
	bootStrapData.DllMainAddress = RESOLVE_RVA(void*,remoteSectionBase, dllToInject->GetDllMainRva());

	remImage->GetRemProcInstance()->WriteBuffer(bootStrapSection,reinterpret_cast<void*>(&bootStrapData), sizeof(bootStrapData));

	remImage->GetRemProcInstance()->WriteBuffer(reinterpret_cast<byte*>(bootStrapSection) + sizeof(bootStrapData) + 20, reinterpret_cast<void*>(&bootStrapper), bootStrapSize);

	if(bootStrapData.DllMainAddress != nullptr)	
	{
		HANDLE bootThreadHandle = remImage->GetRemProcInstance()->CreateThread(reinterpret_cast<byte*>(bootStrapSection) + sizeof(bootStrapData) + 20, reinterpret_cast<BITDYNAMIC>(bootStrapSection));

		WaitForSingleObject(bootThreadHandle, INFINITE);
		DWORD retCode = 0;
		GetExitCodeThread(bootThreadHandle, &retCode);
		remImage->GetRemProcInstance()->FreeSection(bootStrapSection);
		cout << "InjectionThread terminating with Return Code:" << hex << retCode << endl;
	}
	//everything seemed to have been working out.add the injected dll into the known module bases
	//we use the name, that is also used to export stuff, as thats whats important.
	//if the dll exports nothing, we dont naeed the name anyway

	IMAGE_EXPORT_DIRECTORY* localRemImage = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(dllToInject->GetDataDirectoryByIndex(IMAGE_DIRECTORY_ENTRY_EXPORT));
	remImage->AddModuleBaseMapping(bootStrapSection, string(RESOLVE_RVA(char*,dllToInject->GetMappedContent(), localRemImage->Name)));
	cout << "Finished injection" << endl;
}


#pragma region Relocations
void ManualInjector::ResolveRelocations(DllOnDisk*& dllToInject,void* remoteBase)
{
	void* diskImageBase = dllToInject->GetMappedContent();
	int relocDirectorySize = 0;
	BITDYNAMIC imageDelta = reinterpret_cast<BITDYNAMIC>(remoteBase) - dllToInject->GetNtHeader()->OptionalHeader.ImageBase;
	
	//we can use this pointer to iterate, as its a pointer in the local virtual address space and not in the remote space
	IMAGE_BASE_RELOCATION* relocIterator = reinterpret_cast<IMAGE_BASE_RELOCATION*>(dllToInject->GetDataDirectoryBaseByIndex(IMAGE_DIRECTORY_ENTRY_BASERELOC,&relocDirectorySize));
	//getting to the start of the block after the blockInformation
	
	while (relocDirectorySize > 0)
	{
		ResolveRelocBlock(relocIterator, imageDelta, dllToInject);
		relocDirectorySize -= relocIterator->SizeOfBlock;
		//get to the next block
		relocIterator = RESOLVE_RVA(IMAGE_BASE_RELOCATION*, relocIterator, relocIterator->SizeOfBlock);
	}
}

void ManualInjector::ResolveRelocBlock(IMAGE_BASE_RELOCATION* blockHeader, BITDYNAMIC imageDelta, DllOnDisk*& dllToInject)
{
	IMAGE_BASE_RELOCATION* relocBlockHeader = reinterpret_cast<IMAGE_BASE_RELOCATION*>(blockHeader);
	WORD* relocEntryBase = RESOLVE_RVA(WORD*,relocBlockHeader,sizeof(IMAGE_BASE_RELOCATION));
	int entryCount = (relocBlockHeader->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);

	for(int i = 0; i < entryCount;i++)
	{
		ResolveRelocEntry(&(relocEntryBase[i]),relocBlockHeader->VirtualAddress,imageDelta, dllToInject);
	}
}

void ManualInjector::ResolveRelocEntry(void* blockEntry,int pageOffset, BITDYNAMIC imageDelta, DllOnDisk*& dllToInject)
{
	WORD relocType = RELOCATION_TYPE(*reinterpret_cast<WORD*>(blockEntry));
	WORD relocOffset = RELOCATION_OFFSET(*reinterpret_cast<WORD*>(blockEntry));

	if(relocType == IMAGE_REL_BASED_ABSOLUTE)
	{
		//is just a padding reloc entry, no action needed
		return;
	}
	if(relocType != IMAGE_REL_BASED_HIGHLOW &&
		relocType != IMAGE_REL_BASED_DIR64)
	{
		throw string("Not supported relocation type found, cant relocate image properly");
	}
	BITDYNAMIC* writePointer = RESOLVE_RVA(BITDYNAMIC*, dllToInject->GetMappedContent(), pageOffset);
	writePointer = RESOLVE_RVA(BITDYNAMIC*, writePointer, relocOffset);
	(*writePointer) += imageDelta;
}


#pragma endregion

#pragma region Imports
void ManualInjector::ResolveImports(DllOnDisk*& dllToInject)
{
	string currentImportDll;
	IMAGE_IMPORT_DESCRIPTOR* importIterator = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(dllToInject->GetDataDirectoryBaseByIndex(IMAGE_DIRECTORY_ENTRY_IMPORT, nullptr));
	currentImportDll = string(reinterpret_cast<char*>(dllToInject->ResolveRvaInMappedDll(importIterator->Name)));

	//load the dll in te process, if it ist already loaded
	if(remImage->GetRemoteModuleBase(currentImportDll) == reinterpret_cast<void*>(-1))
	{
		DllSearcher* dlls = new DllSearcher();
		auto recDll = dlls->ReadDllFromPathList(currentImportDll);
		this->InjectDll(recDll);
	}
	while(importIterator->Characteristics != 0)
	{
		ImportSingleDll(dllToInject, currentImportDll, importIterator);
		importIterator++;
		currentImportDll = string(reinterpret_cast<char*>(dllToInject->ResolveRvaInMappedDll(importIterator->Name)));
	}
	
}

void ManualInjector::ImportSingleDll(DllOnDisk*& dllToInject, string dllName, IMAGE_IMPORT_DESCRIPTOR* importIterator)
{
	IMAGE_IMPORT_BY_NAME* currentNamedImport = nullptr;
	string functionToImport;
	BITDYNAMIC* dllImports = reinterpret_cast<BITDYNAMIC*>(dllToInject->ResolveRvaInMappedDll(importIterator->OriginalFirstThunk));
	BITDYNAMIC* addressToFill = reinterpret_cast<BITDYNAMIC*>(dllToInject->ResolveRvaInMappedDll(importIterator->FirstThunk));

	//iterating over the offsets to the hint/nameTables
	while (*dllImports != 0)
	{
		if (IMAGE_SNAP_BY_ORDINAL(*dllImports))
		{
			*addressToFill = reinterpret_cast<BITDYNAMIC>(procEx->GetProcAddress(dllName, static_cast<WORD>(*dllImports)));
		}
		else
		{
			currentNamedImport = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(dllToInject->ResolveRvaInMappedDll(*dllImports));
			functionToImport = string(currentNamedImport->Name);
			//a dot in the imported functionname means, that we aredealing with
			//a forwarded import. e.g NTDLL.FreeTls
			//the part in front of the . is the dll name to import from and 
			//the part after the . is the functionname to import
			if (functionToImport.find('.') != string::npos)
			{
				cout << "Found forwarded import in " << dllName << ". Imported name:" << functionToImport << endl;
				string dllName = functionToImport.substr(0, functionToImport.find('.'));
				string funcName = functionToImport.substr(functionToImport.find('.') + 1, functionToImport.length() - functionToImport.find('.'));
				*addressToFill = reinterpret_cast<BITDYNAMIC>(procEx->GetProcAddress(dllName, funcName));
			}
			else
			{
				*addressToFill = reinterpret_cast<BITDYNAMIC>(procEx->GetProcAddress(dllName, functionToImport));
			}
		}
		dllImports++;
		addressToFill++;
	}
}
#pragma endregion
