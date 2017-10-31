#include <iostream>
#include <memory>

#include "RemoteProcess.h"
#include "DllOnDisk.h"
#include "Util.h"
#include "RemoteImage.h"
#include "ManualInjector.h"
#include "DllSearcher.h"
int main()
{
	try
	{
		RemoteImage* remImage = new RemoteImage();
		remImage->OpenRemoteProcess("procexp.exe");
		DllOnDisk* dll = new DllOnDisk();
		dll->LoadDllFromDisk("C:\\hw.dll");

		ManualInjector* mi = new ManualInjector(remImage);

		mi->InjectDll(dll);

		cout << "blub" << endl;
	}
	catch(string message)
	{
		cout << message << endl;
	}
	getchar();
	return 0;
}

