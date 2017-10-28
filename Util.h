#pragma once
#include <cstdlib>

#define UNIQUE_MANAGED(dataType) unique_ptr<Util::ManagedBuffer<dataType>>

class Util
{
public:
	Util();
	~Util();
	
	template <class varType>
	class ManagedBuffer
	{
		varType content;
		int bufferSize;
	public:
		ManagedBuffer();
		~ManagedBuffer();
		void SetContent(varType contentAddress);
		varType GetContent();

		int GetBufferSize();
		void SetBufferSize(int size);
	};
};


#pragma region Managed Buffer
template <class varType>
Util::ManagedBuffer<varType>::ManagedBuffer()
{
	content = nullptr;
	bufferSize = -1;
}

template <class varType>
Util::ManagedBuffer<varType>::~ManagedBuffer()
{
	if (content != nullptr)
	{
		free(content);
	}
}

template <class varType>
void Util::ManagedBuffer<varType>::SetContent(varType contentAddress)
{
	content = contentAddress;
}

template <class varType>
varType Util::ManagedBuffer<varType>::GetContent()
{
	return content;
}

template <class varType>
int Util::ManagedBuffer<varType>::GetBufferSize()
{
	return bufferSize;
}

template <class varType>
void Util::ManagedBuffer<varType>::SetBufferSize(int size)
{
	bufferSize = size;
}
#pragma endregion
