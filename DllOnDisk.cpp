#include "DllOnDisk.h"
#include <iostream>

DllOnDisk::DllOnDisk()
{
	this->rawFile = nullptr;
	this->mappedFileContent = nullptr;
	this->isFileMapped = false;
}

DllOnDisk::~DllOnDisk()
{
	if (this->rawFile != nullptr)
	{
		delete this->rawFile;
	}
	if (this->mappedFileContent != nullptr)
	{
		VirtualFree(this->mappedFileContent,
		            0,
		            MEM_RELEASE);
	}
}

IMAGE_DOS_HEADER* DllOnDisk::GetDosHeader()
{
	CheckFileMapped();
	return reinterpret_cast<IMAGE_DOS_HEADER*>(this->mappedFileContent);
}

IMAGE_NT_HEADERS* DllOnDisk::GetNtHeader()
{
	CheckFileMapped();
	IMAGE_DOS_HEADER* tempHeader = GetDosHeader();
	return RESOLVE_RVA(IMAGE_NT_HEADERS*, tempHeader, tempHeader->e_lfanew);
}

IMAGE_SECTION_HEADER* DllOnDisk::GetSectionHeaderByIndex(const int sectionIndex)
{
	CheckFileMapped();
	IMAGE_NT_HEADERS* ntHeader = GetNtHeader();

	if (sectionIndex < 0 || ntHeader->FileHeader.NumberOfSections <= sectionIndex)
	{
		throw string("Desired sectionbase is out of bounds for this image");
	}
	IMAGE_SECTION_HEADER* firstSection = IMAGE_FIRST_SECTION(ntHeader);
	return &(firstSection[sectionIndex]);
}

void* DllOnDisk::GetSectionBaseByIndex(const int sectionIndex, int* sizeOfDataDirectory)
{
	CheckFileMapped();
	IMAGE_SECTION_HEADER* sectionHeader = GetSectionHeaderByIndex(sectionIndex);

	if (sizeOfDataDirectory != nullptr)
	{
		*sizeOfDataDirectory = sectionHeader->Misc.VirtualSize;
	}
	return RESOLVE_RVA(void*, this->mappedFileContent, sectionHeader->VirtualAddress);
}

IMAGE_DATA_DIRECTORY* DllOnDisk::GetDataDirectoryByIndex(const unsigned int dataDirectoryIndex)
{
	CheckFileMapped();
	IMAGE_NT_HEADERS* ntHeader = GetNtHeader();
	if (ntHeader->OptionalHeader.NumberOfRvaAndSizes <= dataDirectoryIndex)
	{
		throw string("Desired DataDirectory is out of bounds for the given image");
	}
	return reinterpret_cast<IMAGE_DATA_DIRECTORY*>(&ntHeader->OptionalHeader.DataDirectory[dataDirectoryIndex]);
}

void* DllOnDisk::GetDataDirectoryBaseByIndex(const int dataDirectoryIndex, int* sizeOfDataDirectory)
{
	CheckFileMapped();
	IMAGE_DATA_DIRECTORY* ddDir = GetDataDirectoryByIndex(dataDirectoryIndex);
	if (sizeOfDataDirectory != nullptr)
	{
		*sizeOfDataDirectory = ddDir->Size;
	}
	return RESOLVE_RVA(void*, this->mappedFileContent, ddDir->VirtualAddress);
}

Bitvalue DllOnDisk::GetBitness()
{
	CheckFileMapped();
	IMAGE_NT_HEADERS* ntHeader = GetNtHeader();

	if (ntHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
	{
		return IS32BIT;
	}
	if (ntHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
	{
		return IS64BIT;
	}
	return BITERROR;
}

void* DllOnDisk::ResolveRvaInMappedDll(int rva)
{
	return RESOLVE_RVA(void*, this->mappedFileContent, rva);
}

void* DllOnDisk::ResolveRvaInRawFile(int rva)
{
	return RESOLVE_RVA(void*, this->rawFile->GetFilePointer(), rva);
}

void DllOnDisk::PrintDllImports(bool printImportedFunctions)
{
	CheckFileMapped();
	int ddDirSize = 0;
	IMAGE_IMPORT_DESCRIPTOR* dllImports = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(GetDataDirectoryBaseByIndex(
		IMAGE_DIRECTORY_ENTRY_IMPORT, &ddDirSize));
	string currentDllName;


	IMPORT_LOOKUP_ENTRY* iterator = nullptr;
	IAT_ENTRY_TO_FILL iatIterator = nullptr;
	cout << "===========================IMPORTS========================" << endl;
	cout << "Dll imports from the following Dlls" << endl;

	while (dllImports->Characteristics)
	{
		currentDllName = string(reinterpret_cast<char*>(ResolveRvaInMappedDll(dllImports->Name)));
		cout << currentDllName << endl;
		iatIterator = reinterpret_cast<IMPORT_LOOKUP_ENTRY*>(ResolveRvaInMappedDll(dllImports->OriginalFirstThunk));
		if (printImportedFunctions)
		{
			PrintDllImportsOfSingleDll(reinterpret_cast<SWITCH_VALUE*>(iatIterator));
		}
		dllImports++;
	}
	cout << "==========================================================" << endl;
}

void DllOnDisk::PrintDllImportsOfSingleDll(SWITCH_VALUE* importLookUpTable)
{
	IMAGE_IMPORT_BY_NAME* currentNamedImport = nullptr;
	//iterating overtheoffsets to the hint/nameTables
	while (*importLookUpTable != 0)
	{
		if (IMAGE_SNAP_BY_ORDINAL(*importLookUpTable))
		{
			cout << "\tImport by Ordinal" << endl;
		}
		else
		{
			currentNamedImport = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(ResolveRvaInMappedDll(((int)(*importLookUpTable))));
			cout << "\t" << string(currentNamedImport->Name) << endl;
		}
		importLookUpTable++;
	}
}

void DllOnDisk::PrintDllExports()
{
	int exportSize = 0;
	IMAGE_EXPORT_DIRECTORY* exportDirectory = nullptr;
	string currentFunctionName = "";
	DWORD* functionAddressBase = nullptr;
	WORD* functionOrdinalBase = nullptr;
	DWORD* nameAddressBase = nullptr;

	exportDirectory = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(GetDataDirectoryBaseByIndex(
		IMAGE_DIRECTORY_ENTRY_EXPORT, &exportSize));


	functionAddressBase = reinterpret_cast<DWORD*>(ResolveRvaInMappedDll(exportDirectory->AddressOfFunctions));
	functionOrdinalBase = reinterpret_cast<WORD*>(ResolveRvaInMappedDll(exportDirectory->AddressOfNameOrdinals));
	nameAddressBase = reinterpret_cast<DWORD*>(ResolveRvaInMappedDll(exportDirectory->AddressOfNames));

	//are there even any exports?
	if (functionAddressBase == nullptr)
	{
		cout << "There are no exports, that are visible via the Export directory" << endl;
		return;
	}

	cout << "===========================EXPORTS========================" << endl;
	cout << "Dll exports " << exportDirectory->NumberOfFunctions << " of which " << exportDirectory->NumberOfNames <<
		" are exported by name" << endl;

	cout << "RVA\t || FunctionName" << endl;

	//print functions without names first, they are in the front of the func rvas
	unsigned int namelessFuncCount = 0;
	for (namelessFuncCount = 0; namelessFuncCount < exportDirectory->NumberOfFunctions - exportDirectory->NumberOfNames;
	     namelessFuncCount++)
	{
		cout << hex << functionAddressBase[namelessFuncCount] << "\t || [unknown]" << endl;
	}
	for (unsigned int i = 0; i < exportDirectory->NumberOfNames; i++)
	{
		//get the according ordinal to resolve the the used function rva
		cout << hex << functionAddressBase[functionOrdinalBase[i]] << "\t || ";
		currentFunctionName = string(reinterpret_cast<char*>(ResolveRvaInMappedDll(nameAddressBase[i])));
		cout << currentFunctionName << endl;
	}
	cout << "==========================================================" << endl;
}

void DllOnDisk::LoadDllFromDisk(const string dllPath)
{
	rawFile = new DiskFile();

	rawFile->LoadFile(dllPath);

	MapRawFile();
}

void DllOnDisk::MapRawFile()
{
	IMAGE_DOS_HEADER* locDosheader = reinterpret_cast<IMAGE_DOS_HEADER*>(this->rawFile->GetFilePointer());
	IMAGE_NT_HEADERS* locNtHeader = nullptr;
	IMAGE_SECTION_HEADER* sectionHeaderIterator = nullptr;
	void* copySectionBase = nullptr;
	void* destSectionBase = nullptr;

	if (locDosheader->e_magic != DOS_MAGIC)
	{
		throw string("Malformed Dos header detected");
	}
	locNtHeader = RESOLVE_RVA(IMAGE_NT_HEADERS*, locDosheader, locDosheader->e_lfanew);
	if (locNtHeader->Signature != PE_MAGIC)
	{
		throw string("Malformed Pe header detected");
	}
	//alloc place for mapped content
	this->mappedFileContent = reinterpret_cast<byte*>(VirtualAlloc(nullptr,
	                                                               locNtHeader->OptionalHeader.SizeOfImage,
	                                                               MEM_COMMIT | MEM_RESERVE,
	                                                               PAGE_EXECUTE_READWRITE));
	if (this->mappedFileContent == nullptr)
	{
		string ret("Could not allocate section for local mapping. ");
		ret += to_string(GetLastError());
		throw ret;
	}

	//copy headers
	memcpy(this->mappedFileContent,
	       this->rawFile->GetFilePointer(),
	       locNtHeader->OptionalHeader.SizeOfHeaders);

	//copy sections
	sectionHeaderIterator = IMAGE_FIRST_SECTION(locNtHeader);
	for (int i = 0; i < locNtHeader->FileHeader.NumberOfSections; i++)
	{
		copySectionBase = RESOLVE_RVA(void*, this->rawFile->GetFilePointer(), sectionHeaderIterator[i].PointerToRawData);
		destSectionBase = RESOLVE_RVA(void*, this->mappedFileContent, sectionHeaderIterator[i].VirtualAddress);
		memcpy_s(destSectionBase, sectionHeaderIterator[i].SizeOfRawData, copySectionBase,
		         sectionHeaderIterator[i].SizeOfRawData);
	}
	this->isFileMapped = true;
}


void DllOnDisk::CheckFileMapped()
{
	if (!this->isFileMapped)
	{
		throw string("File not mapped. Cant interaact using this method");
	}
}
