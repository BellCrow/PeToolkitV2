#include <iostream>

#include "DllOnDisk.h"
#include "RemoteImage.h"
#include "ManualInjector.h"
#include "DllSearcher.h"
int main()
{
	try
	{
		RemoteImage* remImage = new RemoteImage();
		remImage->OpenRemoteProcess("PEview.exe");
		DllOnDisk* dll = new DllOnDisk();
		dll->LoadDllFromDisk("C:\\hw.dll");

		ManualInjector* mi = new ManualInjector(remImage);

		mi->InjectDll(dll);
	}
	catch(string message)
	{
		cout <<"Exception:" << message << endl;
	}
	getchar();
	return 0;
}

