#pragma once
#include <Windows.h>
#include <string>
#include <Shlwapi.h>

using namespace std;

class DiskFile
{
	byte* rawFileContent;
	int fileSize;
	bool fileLoaded;
	void CheckLoaded();
public:
	DiskFile();
	~DiskFile();
	void LoadFile(string filePath);
	int GetFileSize();
	byte* GetFilePointer();
	bool IsFileLoaded();
};

