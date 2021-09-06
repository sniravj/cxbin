#include "cxutil.h"

namespace cxbin
{
	Chunk::Chunk()
		:chunk(-1)
		, bufferSize(0)
		, buffer(nullptr)
	{

	}
	Chunk::~Chunk()
	{
		if (buffer)
		{
			delete buffer;
			buffer = nullptr;
		}
	}
}