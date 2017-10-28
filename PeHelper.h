#pragma once
#include <Windows.h>


#define RELOCATION_TYPE(relocEntry) ((relocEntry & 0xF000) >> 12)
#define RELOCATION_OFFSET(relocEntry) (relocEntry & 0x0FFF)
#define RESOLVE_RVA(castToType, address, addField) ((castToType)(((byte*)address) +(addField)))

#define PE_MAGIC 0x4550
#define DOS_MAGIC 0x5a4d
#define CHARACTERISTICS_RELOC_STRIPPED_FIELD 0x0001


enum Bitvalue
{
	IS32BIT,
	IS64BIT,
	BITERROR
};

#ifdef _WIN64
	#define IMPORT_LOOKUP_ENTRY __int64 
	#define SWITCH_VALUE __int64
#else
	#define IMPORT_LOOKUP_ENTRY __int32 
	#define SWITCH_VALUE __int32
#endif

#define IAT_ENTRY_TO_FILL void*