#include "reader.h"
#include <zlib.h>
#include "trimesh2/TriMesh.h"

namespace cxbin
{
	bool loadCXBin0(FILE* f, unsigned fileSize, trimesh::TriMesh& out, ccglobal::Tracer* tracer)
	{
		bool success = false;
		int vertNum = 0;
		int faceNum = 0;
		int totalNum = 0;
		uLong compressNum = 0;

		fread(&totalNum, sizeof(int), 1, f);
		fread(&vertNum, sizeof(int), 1, f);
		fread(&faceNum, sizeof(int), 1, f);
		fread(&compressNum, sizeof(uLong), 1, f);

		unsigned char* data = new unsigned char[totalNum];
		unsigned char* cdata = new unsigned char[compressNum];

		uLong uTotalNum = totalNum;
		if (fread(cdata, 1, compressNum, f))
		{
			if (uncompress(data, &uTotalNum, cdata, compressNum) == Z_OK)
			{
				if (vertNum > 0)
				{
					out.vertices.resize(vertNum);
					memcpy(&out.vertices.at(0), data, vertNum * sizeof(trimesh::vec3));
				}
				if (faceNum > 0)
				{
					out.faces.resize(faceNum);
					memcpy(&out.faces.at(0), data + vertNum * sizeof(trimesh::vec3), faceNum * sizeof(trimesh::ivec3));
				}

				success = true;
			}
		}

		delete[]data;
		delete[]cdata;
		return success;
	}

	bool loadCXBin(FILE* f, unsigned fileSize, trimesh::TriMesh& out, ccglobal::Tracer* tracer)
	{
		int version = -1;
		fread(&version, sizeof(int), 1, f);
		char data[12] = "";
		fread(data, 1, 12, f);
		if (version == 0)
			return loadCXBin0(f, fileSize, out, tracer);
		return false;
	}
}