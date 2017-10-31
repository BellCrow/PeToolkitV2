#pragma once
#include "DllOnDisk.h"
#include "RemoteImage.h"
#include "ProcAddressExtractor.h"

typedef struct _BOOTSTRAPPER_DATA
{
	void* lpvReserved;
	BITDYNAMIC fdwReason;
	BITDYNAMIC hInstance;

	void* DllMainAddress;
} BOOTSTRAPPER_DATA;

class ManualInjector
{
	RemoteImage* remImage;
	ProcAddressExtractor* procEx;

#pragma region Relocations
	void ResolveRelocations(DllOnDisk*& dllToInject, void* remoteBase);
	void ResolveRelocBlock(IMAGE_BASE_RELOCATION* blockHeader, BITDYNAMIC imageDelta, DllOnDisk*& dllToInject);
	void ResolveRelocEntry(void* blockEntry, int pageOffset, BITDYNAMIC imageDelta, DllOnDisk*& dllToInject);
#pragma endregion

#pragma region Imports
	void ResolveImports(DllOnDisk*& dllToInject);
	void ManualInjector::ImportSingleDll(DllOnDisk*& dllToInject, string dllName, IMAGE_IMPORT_DESCRIPTOR* importIterator);
#pragma endregion

public:
	ManualInjector(RemoteImage*& processToInjectInto);
	~ManualInjector();
	void InjectDll(DllOnDisk*& dllToInject);
	
};

