#include "reader.h"
#include <zlib.h>
#include "trimesh2/TriMesh.h"

#include "cxbin/convert.h"
#include "ccglobal/tracer.h"
#include "trimesh2/endianutil.h"

namespace cxbin
{
	bool loadCXBin0(FILE* f, size_t fileSize, trimesh::TriMesh& out, ccglobal::Tracer* tracer)
	{
		bool success = false;
		int vertNum = 0;
		int faceNum = 0;
		int totalNum = 0;
		int compressNum = 0;

		bool need_swap = trimesh::we_are_big_endian();
		fread(&totalNum, sizeof(int), 1, f);
		fread(&vertNum, sizeof(int), 1, f);
		fread(&faceNum, sizeof(int), 1, f);
		fread(&compressNum, sizeof(int), 1, f);
		if (need_swap)
		{
			trimesh::swap_int(totalNum);
			trimesh::swap_int(vertNum);
			trimesh::swap_int(faceNum);
			trimesh::swap_int(compressNum);
		}
		unsigned char* data = new unsigned char[totalNum];
		unsigned char* cdata = new unsigned char[compressNum];
		formartPrint(tracer, "loadCXBin0 load %d %d %d %d.", totalNum, (int)compressNum, vertNum, faceNum);

		uLong uTotalNum = (uLong)totalNum;
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

		if (true == success)
		{
			if (tracer)
			{
				tracer->success();
			}
		}
		else
		{
			if (tracer)
			{
				tracer->failed("file read failed");
			}
		}
		return success;
	}

	bool loadCXBin(FILE* f, size_t fileSize, trimesh::TriMesh& out, ccglobal::Tracer* tracer)
	{
		int version = -1;
		fread(&version, sizeof(int), 1, f);

		bool need_swap = trimesh::we_are_big_endian();
		if (need_swap)
			trimesh::swap_int(version);

		char data[12] = "";
		fread(data, 1, 12, f);
		if (version == 0)
			return loadCXBin0(f, fileSize, out, tracer);
		return false;
	}
}