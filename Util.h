#pragma once
#include <cstdlib>

class Util
{
public:
	Util();
	~Util();
	
	class ManagedBuffer
	{
		void* content;
		int bufferSize;
	public:
		ManagedBuffer();
		~ManagedBuffer();
		void SetContent(void* contentAddress);
		void* GetContent();

		int GetBufferSize();
		void SetBufferSize(int size);
	};
};

