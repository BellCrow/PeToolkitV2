#pragma once
#include <string>
#include "DiskFile.h"
#include <Windows.h>
#include "PeHelper.h"
#pragma comment(lib,"Shlwapi.lib")
using namespace std;
class DllOnDisk
{
	bool isFileMapped;
	byte* mappedFileContent;
	DiskFile* rawFile;

	void CheckFileMapped();
	void MapRawFile();
	void PrintDllImportsOfSingleDll(SWITCH_VALUE* importLookUpTable);
public:
	DllOnDisk();
	~DllOnDisk();

	void LoadDllFromDisk(string dllPath);

	IMAGE_DOS_HEADER* GetDosHeader();
	IMAGE_NT_HEADERS* GetNtHeader();

	IMAGE_SECTION_HEADER* GetSectionHeaderByIndex(int sectionIndex);
	void* GetSectionBaseByIndex(int sectionIndex, _In_opt_ int* sizeOfDataDirectory);

	IMAGE_DATA_DIRECTORY* GetDataDirectoryByIndex(int unsigned dataDirectoryIndex);
	void* GetDataDirectoryBaseByIndex(int dataDirectoryIndex, _In_opt_ int* sizeOfDataDirectory);

	Bitvalue GetBitness();
	int AreRelocsStripped();

	void* ResolveRvaInMappedDll(int rva);
	void* ResolveRvaInRawFile(int rva);


	byte* GetMappedContent();
	int GetMappedContentSize();

	//DllInfos
	void PrintDllImports(bool printImportedFunctions);
	void PrintDllExports();

	//dll searcher

};

