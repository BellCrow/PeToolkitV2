#include "RemoteProcess.h"
#include "DllOnDisk.h"

#include <iostream>
#include <memory>
#include "Util.h"
#include "RemoteImage.h"

int main()
{
	try
	{
		DllOnDisk* ddod = new DllOnDisk();
		ddod->LoadDllFromDisk("C:\\kernel32.dll");
		ddod->PrintDllImports(true);
		ddod->PrintDllExports();
	}
	catch(string message)
	{
		cout << message << endl;
	}
	getchar();
	return 0;
}

