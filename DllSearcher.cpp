#include "DllSearcher.h"
DllSearcher::DllSearcher(string pathListPath)
{
	Init(pathListPath);
}

//this constructor will simply search in the current working dir for the config file
DllSearcher::DllSearcher()
{
	CHAR cwd[MAX_PATH] = { 0 };
	GetCurrentDirectory(MAX_PATH, cwd);
	Init(string(cwd) +'\\'+ DEFAULT_CONFIG_NAME);
}

void DllSearcher::Init(string pathListPath)
{
	string readAdder;
	int trimStart = 0;
	int trimEnd = 0;
	int validityLineNumber = 0;

	searchPaths = new vector<string>;

	if (!PathFileExists(pathListPath.c_str()))
	{
		throw string("Could not find pathList file in given path");
	}

	//load the path list
	ifstream configLoader(pathListPath);

	while (getline(configLoader, readAdder))
	{
		if (readAdder.empty())
		{
			continue;
		}

		//cut out trailing and prepending whitespaces
		trimStart = readAdder.find_first_not_of(' ');
		trimEnd = readAdder.find_last_not_of(' ');
		readAdder = readAdder.substr(trimStart, trimEnd + 1);
		//check if its a comment line
		if (!readAdder.find_first_of(COMMENTCHAR) == 0)
		{
			//we want to add the cwd also?
			if (readAdder == CWD_KEYWORD)
			{
				CHAR cwd[MAX_PATH] = { 0 };
				GetCurrentDirectory(MAX_PATH, cwd);
				searchPaths->push_back(string(cwd));
			}
			else
			{
				//check if the path is even valid
				if (!PathFileExists(readAdder.c_str()))
				{
					throw (string("A given path in the searchpath config is not a valid path.(Hint: The PathFileExists winapi function is used to check validity). Line:") + to_string(validityLineNumber));
				}
				searchPaths->push_back(readAdder);
			}
		}
		validityLineNumber++;
	}
}



DllSearcher::~DllSearcher()
{
	delete searchPaths;
}

DllOnDisk* DllSearcher::ReadDllFromPathList(string dllName)
{

	DllOnDisk* loadedDll = new DllOnDisk();
	char foundFilePath[MAX_PATH] = { 0 };

	for(vector<string>::iterator it = searchPaths->begin(); it != searchPaths->end();++it)
	{
		if(SearchPath(it->c_str(),
			dllName.c_str(),
			nullptr,
			MAX_PATH,
			foundFilePath,
			nullptr) != 0)
		{
			//found a fitting file in one of the directories
			loadedDll->LoadDllFromDisk(string(foundFilePath));
			return loadedDll;
		}

	}
	//the dll could not be found in any known directory;
	throw (string("Could not find missing dll ind any given searchpath: ") + dllName);
	return nullptr;
}
