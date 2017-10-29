#include "ManualInjector.h"
#include "Util.h"
#include <iostream>


ManualInjector::ManualInjector(RemoteImage*& processToInjectInto)
{
	remImage = processToInjectInto;
}


ManualInjector::~ManualInjector()
{
	delete remImage;
}

void ManualInjector::InjectDll(DllOnDisk*& dllToInject)
{
	void* remoteSectionBase = nullptr;

	//reserve the neccesary space in the remoteImage
	remoteSectionBase = remImage->AllocSpaceForDll(dllToInject);

	ResolveRelocations(dllToInject, remoteSectionBase);
}

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
