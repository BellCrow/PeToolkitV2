#pragma once
#include <vector>
#include <Windows.h>
#include <fstream>
#include "DllOnDisk.h"


#define COMMENTCHAR '#'
#define CWD_KEYWORD "cwd"
#define DEFAULT_CONFIG_NAME "dllSearchPaths.ini"
using namespace std;
class DllSearcher
{
	vector<string>* searchPaths;
	void Init(string pathListPath);
public:
	DllSearcher(string pathListPath);
	DllSearcher();
	~DllSearcher();

	DllOnDisk* ReadDllFromPathList(string dllName);


};

