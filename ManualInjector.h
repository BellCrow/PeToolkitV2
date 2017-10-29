#pragma once
#include "DllOnDisk.h"
#include "RemoteImage.h"

class ManualInjector
{
	RemoteImage* remImage;

	void ResolveRelocations(DllOnDisk*& dllToInject, void* remoteBase);
#pragma region reloc helper
	void ResolveRelocBlock(IMAGE_BASE_RELOCATION* blockHeader, BITDYNAMIC imageDelta, DllOnDisk*& dllToInject );
	void ResolveRelocEntry(void* blockEntry, int pageOffset, BITDYNAMIC imageDelta, DllOnDisk*& dllToInject);
#pragma endregion
public:
	ManualInjector(RemoteImage*& processToInjectInto);
	~ManualInjector();
	void InjectDll(DllOnDisk*& dllToInject);
};

