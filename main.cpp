#include <iostream>
#include <memory>

#include "RemoteProcess.h"
#include "DllOnDisk.h"
#include "Util.h"
#include "RemoteImage.h"

int main()
{
	try
	{
		RemoteImage* remImage = new RemoteImage();
		remImage->OpenRemoteProcess("chrome.exe");
		void* remSectionBase = nullptr;
		auto sectionContent = remImage->GetDataDirectoryContentByIndex("kernel32.dll", 6, &remSectionBase);
		cout << "blub" << endl;
	}
	catch(string message)
	{
		cout << message << endl;
	}
	getchar();
	return 0;
}

