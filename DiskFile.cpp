#include "DiskFile.h"



DiskFile::DiskFile()
{
	this->rawFileContent = nullptr;
	this->fileSize = -1;
	this->fileLoaded = false;
}

DiskFile::~DiskFile()
{
	if (rawFileContent != nullptr)
	{
		free(this->rawFileContent);
	}
}

void DiskFile::CheckLoaded()
{
	if (!this->fileLoaded)
	{
		throw string("File not loaded. Cant interact with content");
	}
}

void DiskFile::LoadFile(string filePath)
{
	HANDLE fileLoadHandle = INVALID_HANDLE_VALUE;
	LARGE_INTEGER tempFileSize = {0};

	if(!PathFileExists(filePath.c_str()))
	{
		throw string("Desired File does not exists");
	}

	fileLoadHandle = CreateFile(filePath.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if(fileLoadHandle == INVALID_HANDLE_VALUE)
	{
		string retString("Found file, but errors opening it.");
		retString += to_string(GetLastError());
		throw retString;
	}

	if(!GetFileSizeEx(fileLoadHandle,&tempFileSize))
	{
		CloseHandle(fileLoadHandle);
		throw string("Error getting the file size");
	}

	if(tempFileSize.LowPart == 0)
	{
		CloseHandle(fileLoadHandle);
		throw string("Error. File size is 0");
	}
	this->rawFileContent = reinterpret_cast<byte*>(calloc(tempFileSize.LowPart, sizeof(byte)));

	if (!ReadFile(fileLoadHandle,
		this->rawFileContent,
		tempFileSize.LowPart,
		nullptr,
		nullptr))
	{
		free(this->rawFileContent);
		this->rawFileContent = nullptr;
		CloseHandle(fileLoadHandle);
		throw string("Error. File size is 0");
	}
	this->fileLoaded = true;
	this->fileSize = tempFileSize.LowPart;
	CloseHandle(fileLoadHandle);
}

int DiskFile::GetFileSize()
{
	CheckLoaded();
	return this->fileSize;
}

byte* DiskFile::GetFilePointer()
{
	CheckLoaded();
	return this->rawFileContent;
}

bool DiskFile::IsFileLoaded()
{
	return this->fileLoaded;
}
