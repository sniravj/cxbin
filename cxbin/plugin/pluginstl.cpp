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
	StlSaver::StlSaver()
	{

	}

	StlSaver::~StlSaver()
	{

	}

	std::string StlSaver::expectExtension()
	{
		return "stl";
	}

    bool write_stl_w(trimesh::TriMesh* mesh, FILE* f)
    {
#if defined(__ANDROID__)
        LOGI("write_stl.\n");
#endif
        bool need_swap = trimesh::we_are_big_endian();
        char header[80];
        memset(header, ' ', 80);
        FWRITE(header, 80, 1, f);

        int nfaces = mesh->faces.size();
        if (need_swap)
            trimesh::swap_int(nfaces);
        FWRITE(&nfaces, 4, 1, f);

        for (size_t i = 0; i < mesh->faces.size(); i++) {
            float fbuf[12];
            trimesh::vec tn = mesh->trinorm(i);
            trimesh::normalize(tn);
            fbuf[0] = tn[0]; fbuf[1] = tn[1]; fbuf[2] = tn[2];
            fbuf[3] = mesh->vertices[mesh->faces[i][0]][0];
            fbuf[4] = mesh->vertices[mesh->faces[i][0]][1];
            fbuf[5] = mesh->vertices[mesh->faces[i][0]][2];
            fbuf[6] = mesh->vertices[mesh->faces[i][1]][0];
            fbuf[7] = mesh->vertices[mesh->faces[i][1]][1];
            fbuf[8] = mesh->vertices[mesh->faces[i][1]][2];
            fbuf[9] = mesh->vertices[mesh->faces[i][2]][0];
            fbuf[10] = mesh->vertices[mesh->faces[i][2]][1];
            fbuf[11] = mesh->vertices[mesh->faces[i][2]][2];
            if (need_swap) {
                for (int j = 0; j < 12; j++)
                    trimesh::swap_float(fbuf[j]);
            }
            FWRITE(fbuf, 48, 1, f);
            unsigned char att[2] = { 0, 0 };
            FWRITE(att, 2, 1, f);
        }
        return true;
    }

	bool StlSaver::save(FILE* f, trimesh::TriMesh* out, ccglobal::Tracer* tracer, std::string fileName)
	{
		//return out->write(f,fileName.c_str());
        return write_stl_w(out,f);
	}

	// A valid binary STL buffer should consist of the following elements, in order:
	// 1) 80 byte header
	// 2) 4 byte face count
	// 3) 50 bytes per face
	static bool IsBinarySTL(FILE* f, size_t fileSize) {
		if (fileSize < 84) {
			return false;
		}

		fseek(f, 80L, SEEK_SET);
		size_t faceCount(0);
		fread(&faceCount, sizeof(int), 1, f);
		const size_t expectedBinaryFileSize = faceCount * 50 + 84;

		if (expectedBinaryFileSize == fileSize) {
			return true;
		}
		if (fileSize - expectedBinaryFileSize < 64) {
			size_t cha = fileSize - expectedBinaryFileSize;
			fseek(f, expectedBinaryFileSize, SEEK_SET);
			char* buf = new char[cha];
			fread(buf, cha, 1, f);

			bool is_stl = true;
			for (int i = 0; i < cha; i++) {
				if (buf[i] != 0x0) {
					is_stl = false;
					break;
				}
			}

			delete[] buf;
			buf = nullptr;

			return is_stl;
		}

		if (fileSize > expectedBinaryFileSize)
			return true;

		return false;
	}

	// An ascii STL buffer will begin with "solid NAME", where NAME is optional.
	// Note: The "solid NAME" check is necessary, but not sufficient, to determine
	// if the buffer is ASCII; a binary header could also begin with "solid NAME".
	static bool IsAsciiSTL(FILE* f, size_t fileSize) {
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

		//判断模型格式是不是带BOM的UTF8文件
		fseek(f, nNumWhiteSpace, SEEK_SET);
		char buffer[3];
		if (fread(buffer, 3, 1, f) != 1)
		{
			return false;
		}
		else
		{
			bool bRet = buffer[0] == (char)0xEF && buffer[1] == (char)0xBB && buffer[2] == (char)0xBF;
			if (bRet)
			{
				fseek(f, nNumWhiteSpace+4, SEEK_SET);
			}
			else
			{
				fseek(f, nNumWhiteSpace, SEEK_SET);
			}
		}
		
		
		char chBuffer[6];
		if (fread(chBuffer, 5, 1, f) != 1)
			return false;

		chBuffer[5] = '\0';
		bool isASCII = false;
		if (!strcmp(chBuffer, "solid") || !strcmp(chBuffer, "Solid") || !strcmp(chBuffer, "SOLID"))
			isASCII = true;

		fseek(f, 0L, SEEK_SET);
		unsigned char b[1];
		if (isASCII) {
			// A lot of importers are write solid even if the file is binary. So we have to check for ASCII-characters.
			if (fileSize > 5) {//500  size(solid) bug for 6.1_棱锥-J.stl
				isASCII = true;
				int detect = fileSize > 500 ? 500 : fileSize;//500   bug for 6.1_棱锥-J.stl
				for (unsigned int i = 0; i < detect; i++) {//500   bug for 6.1_棱锥-J.stl

					fread(b, 1, 1, f);
					if (b[0] > 127) {
						isASCII = false;
						break;
					}
				}

				if (isASCII == false)
				{
					fseek(f, 0L, SEEK_SET);
					char line[1024] = { '\0' };
					bool findEndfacet = false;
					bool findVertex = false;
					for (int i = 0; i < 20; ++i)
					{
						if (findEndfacet && findVertex)
						{
							isASCII = true;
							break;
						}
						fgets(line, 1024, f);
						std::string sourceLine(line);
						if (sourceLine.find("endfacet") != std::string::npos)
						{
							findEndfacet = true;
							continue;
						}
						if (sourceLine.find("vertex") != std::string::npos)
						{
							findVertex = true;
							continue;
						}
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

		size_t curSize = 0;
		size_t deltaSize = fileSize / 10;
		size_t nextSize = 0;

		size_t interSize = fileSize / 100;
		size_t iNextSize = 0;
		
		bool parseError = false;
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

				SESSION_TICK("text-stl");
				nextSize += deltaSize;
			}

			if (curSize > iNextSize)
			{
				if (tracer && tracer->interrupt())
					return false;

				iNextSize += interSize;
			}
			
			if (sourceLine.find("vertex") != std::string::npos)
			{
				sourceLine = stringutil::trimHeadTail(sourceLine);
				std::vector<std::string> segs = stringutil::splitString(sourceLine, " ");
				if (segs.size() != 4)
				{
					parseError = true;
					break;
				}
				float x = atof(segs.at(1).c_str());
				float y = atof(segs.at(2).c_str());
				float z = atof(segs.at(3).c_str());
				out.vertices.push_back(trimesh::vec3(x, y, z));
			}
		}

		if (parseError)
			out.vertices.clear();
		int face = out.vertices.size() / 3;
		if(face > 0)
			out.faces.resize(face);
		for (int i = 0; i < face; ++i)
		{
			trimesh::ivec3& tri = out.faces.at(i);
			tri.x = 3 * i;
			tri.y = 3 * i + 1;
			tri.z = 3 * i + 2;
		}
		SESSION_TICK("text-stl");

		return face > 0;
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

			if (tracer && nfacets >10 && i % calltime == 1)
			{
				tracer->progress((float)i / (float)nfacets);
				SESSION_TICK("binary-stl");
			}
			if (tracer && nfacets >100 &&  i % interTimes == 1 && tracer->interrupt())
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

		SESSION_TICK("binary-stl");
		LOGI("parse binary stl success...\n");
		return nfacets > 0;
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

	bool StlLoader::tryLoad(FILE* f, size_t fileSize)
	{
		return IsAsciiSTL(f, fileSize) || IsBinarySTL(f, fileSize);
	}

	bool StlLoader::load(FILE* f, size_t fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
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