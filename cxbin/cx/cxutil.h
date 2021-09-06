#ifndef CXBIN_CXUTIL_1630913098859_H
#define CXBIN_CXUTIL_1630913098859_H
#include <vector>

namespace cxbin
{
	static int version = 0;
	static int invalid = -1;

	class Chunk
	{
	public:
		Chunk();
		virtual ~Chunk();

		int chunk;
		int bufferSize;
		unsigned char* buffer;
	};

	
}

#endif // CXBIN_CXUTIL_1630913098859_H