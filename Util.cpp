#include "Util.h"

Util::Util()
{
}


Util::~Util()
{
}


#pragma region Managed Buffer
Util::ManagedBuffer::ManagedBuffer()
{
	content = nullptr;
	bufferSize = -1;
}

Util::ManagedBuffer::~ManagedBuffer()
{
	if(content != nullptr)
	{
		free(content);	
	}
}

void Util::ManagedBuffer::SetContent(void* contentAddress)
{
	content = contentAddress;
}

void* Util::ManagedBuffer::GetContent()
{
	return content;
}

int Util::ManagedBuffer::GetBufferSize()
{
	return bufferSize;
}

void Util::ManagedBuffer::SetBufferSize(int size)
{
	bufferSize = size;
}
#pragma endregion