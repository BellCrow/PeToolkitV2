#pragma once
#include "DllOnDisk.h"
#include "RemoteImage.h"
#include "ProcAddressExtractor.h"

class ManualInjector
{
	RemoteImage* remImage;
	ProcAddressExtractor* procEx;

#pragma region Relocations
	void ResolveRelocations(DllOnDisk*& dllToInject, void* remoteBase);
	void ResolveRelocBlock(IMAGE_BASE_RELOCATION* blockHeader, BITDYNAMIC imageDelta, DllOnDisk*& dllToInject );
	void ResolveRelocEntry(void* blockEntry, int pageOffset, BITDYNAMIC imageDelta, DllOnDisk*& dllToInject);
#pragma endregion

#pragma region Imports
	void ResolveImports(DllOnDisk*& dllToInject);
	void ImportSingleDll(DllOnDisk*& dllToInject, BITDYNAMIC* dllImports);
#pragma endregion

public:
	ManualInjector(RemoteImage*& processToInjectInto);
	~ManualInjector();
	void InjectDll(DllOnDisk*& dllToInject);
};

