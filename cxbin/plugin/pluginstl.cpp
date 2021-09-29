#include "pluginstl.h"
#include "ccglobal/log.h"
#include "ccglobal/tracer.h"
#include "ccglobal/spycc.h"

#include "stringutil/util.h"
#include "stringutil/filenameutil.h"
#include "trimesh2/endianutil.h"
#include "trimesh2/TriMesh.h"

namespace cxbin
{
	// A valid binary STL buffer should consist of the following elements, in order:
	// 1) 80 byte header
	// 2) 4 byte face count
	// 3) 50 bytes per face
	static bool IsBinarySTL(FILE* f, unsigned int fileSize) {
		if (fileSize < 84) {
			return false;
		}

		fseek(f, 80L, SEEK_SET);
		uint32_t faceCount(0);
		fread(&faceCount, sizeof(uint32_t), 1, f);
		const uint32_t expectedBinaryFileSize = faceCount * 50 + 84;

		return expectedBinaryFileSize == fileSize;
	}

	// An ascii STL buffer will begin with "solid NAME", where NAME is optional.
	// Note: The "solid NAME" check is necessary, but not sufficient, to determine
	// if the buffer is ASCII; a binary header could also begin with "solid NAME".
	static bool IsAsciiSTL(FILE* f, unsigned int fileSize) {
		fseek(f, 0L, SEEK_SET);
		int nNumWhiteSpace = 0;
		unsigned char nWhiteSpace;
		if (fread(&nWhiteSpace, 1, 1, f) != 1)
			return false;

		while (isspace(nWhiteSpace))
		{
			++nNumWhiteSpace;
			if (fread(&nWhiteSpace, 1, 1, f) != 1)
				return false;
		}

		fseek(f, nNumWhiteSpace, SEEK_SET);
		char chBuffer[6];
		if (fread(chBuffer, 5, 1, f) != 1)
			return false;

		chBuffer[5] = '\0';
		bool isASCII = false;
		if (!strcmp(chBuffer, "solid") || !strcmp(chBuffer, "Solid") || !strcmp(chBuffer, "SOLID"))
			isASCII = true;

		fseek(f, 0L, SEEK_SET);
		char b[1];
		if (isASCII) {
			// A lot of importers are write solid even if the file is binary. So we have to check for ASCII-characters.
			if (fileSize >= 500) {
				isASCII = true;
				for (unsigned int i = 0; i < 500; i++) {

					fread(b, 1, 1, f);
					if (b[0] > 127) {
						isASCII = false;
						break;
					}
				}
			}
		}
		return isASCII;
	}

	bool read_stl_text(FILE* f, unsigned int fileSize, trimesh::TriMesh& out, ccglobal::Tracer* tracer)
	{
		fseek(f, 0L, SEEK_SET);
		char line[1024] = { '\0' };

		int face = 0;
		int n = 0;

		size_t curSize = 0;
		size_t deltaSize = fileSize / 10;
		size_t nextSize = 0;

		size_t interSize = fileSize / 100;
		size_t iNextSize = 0;

		while (!feof(f))
		{
			fgets(line, 1024, f);

			std::string sourceLine(line);

			curSize += strlen(line);
			if (curSize > nextSize)
			{
				if (tracer)
				{
					LOGI("parse text stl bytes %d\n", (int)curSize);
					tracer->progress((float)curSize / (float)fileSize);
				}

				SESSION_TICK("text-stl")
				nextSize += deltaSize;
			}

			if (curSize > iNextSize)
			{
				if (tracer && tracer->interrupt())
					return false;

				iNextSize += interSize;
			}

			sourceLine = stringutil::trimHeadTail(sourceLine);

			std::vector<std::string> segs = stringutil::splitString(sourceLine, " ");

			if (segs.size() == 4 && !strcmp(segs.at(0).c_str(), "vertex"))
			{
				float x = atof(segs.at(1).c_str());
				float y = atof(segs.at(2).c_str());
				float z = atof(segs.at(3).c_str());
				out.vertices.push_back(trimesh::vec3(x, y, z));
				++n;
			}
		}

		face = n / 3;
		out.faces.resize(face);
		for (int i = 0; i < face; ++i)
		{
			trimesh::ivec3& tri = out.faces.at(i);
			tri.x = 3 * i;
			tri.y = 3 * i + 1;
			tri.z = 3 * i + 2;
		}
		SESSION_TICK("text-stl")

		return true;
	}

	// Read a binary STL file
	bool read_stl_binary(FILE* f, size_t fileSize, trimesh::TriMesh& out, ccglobal::Tracer* tracer)
	{
		fseek(f, 0, SEEK_SET);
		bool need_swap = trimesh::we_are_big_endian();

		char header[80];
		COND_READ(true, header, 80);

		int nfacets;
		COND_READ(true, nfacets, 4);
		if (need_swap)
			trimesh::swap_int(nfacets);

		LOGI("parse binary stl ...face %d\n", nfacets);

		int calltime = nfacets / 10;
		int interTimes = nfacets / 100;
		if (calltime <= 0)
			calltime = nfacets;
		for (int i = 0; i < nfacets; i++) {

			if (tracer && i % calltime == 1)
			{
				tracer->progress((float)i / (float)nfacets);
				SESSION_TICK("binary-stl")
			}
			if (tracer && i % interTimes == 1 && tracer->interrupt())
				return false;

			float fbuf[12];
			COND_READ(true, fbuf, 48);
			if (need_swap) {
				for (int j = 3; j < 12; j++)
					trimesh::swap_float(fbuf[j]);
			}
			int v = out.vertices.size();
			out.vertices.push_back(trimesh::vec3(fbuf[3], fbuf[4], fbuf[5]));
			out.vertices.push_back(trimesh::vec3(fbuf[6], fbuf[7], fbuf[8]));
			out.vertices.push_back(trimesh::vec3(fbuf[9], fbuf[10], fbuf[11]));
			out.faces.push_back(trimesh::ivec3(v, v + 1, v + 2));
			unsigned char att[2];
			COND_READ(true, att, 2);
		}

		SESSION_TICK("binary-stl")
		LOGI("parse binary stl success...\n");
		return true;
	}


	StlLoader::StlLoader()
	{

	}

	StlLoader::~StlLoader()
	{

	}

	std::string StlLoader::expectExtension()
	{
		return "stl";
	}

	bool StlLoader::tryLoad(FILE* f, unsigned fileSize)
	{
		return IsAsciiSTL(f, fileSize) || IsBinarySTL(f, fileSize);
	}

	bool StlLoader::load(FILE* f, unsigned fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
	{
		trimesh::TriMesh* model = new trimesh::TriMesh();
		out.push_back(model);

		bool success = false;
		if (IsAsciiSTL(f, fileSize))
			return read_stl_text(f, fileSize, *out[0], tracer);
		else if (IsBinarySTL(f, fileSize))
			return read_stl_binary(f, fileSize, *out[0], tracer);

		if (tracer)
		{
			tracer->success();
		}
		return success;
	}
}