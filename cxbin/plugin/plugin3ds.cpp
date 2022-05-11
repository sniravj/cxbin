#include "plugin3ds.h"

#include "trimesh2/TriMesh.h"
#include "trimesh2/endianutil.h"
#include "stringutil/filenameutil.h"
#include "cxbin/impl/inner.h"
#include "ccglobal/tracer.h"

#define CHUNK_3DS_MAIN  0x4D4Du
#define CHUNK_3DS_MODEL 0x3D3Du
#define CHUNK_3DS_OBJ   0x4000u
#define CHUNK_3DS_MESH  0x4100u
#define CHUNK_3DS_VERT  0x4110u
#define CHUNK_3DS_FACE  0x4120u

namespace cxbin
{
	_3dsLoader::_3dsLoader()
	{

	}

	_3dsLoader::~_3dsLoader()
	{

	}

	std::string _3dsLoader::expectExtension()
	{
		return "3ds";
	}

	bool _3dsLoader::tryLoad(FILE* f, size_t fileSize)
	{
		int c = fgetc(f);
		if (c == EOF) {
			return false;
		}

		if (c == 0x4d) {
			int c2 = fgetc(f);
			ungetc(c2, f);
			ungetc(c, f);
			if (c2 == 0x4d)
				return true;
		}

		return false;
	}

	bool _3dsLoader::load(FILE* f, size_t fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
	{
		bool need_swap = trimesh::we_are_big_endian();
		int mstart = 0;

		trimesh::TriMesh* model = new trimesh::TriMesh();
		out.push_back(model);

		while (1) {
			if (tracer && tracer->interrupt())
				return false;

			unsigned short chunkid;
			unsigned chunklen;
			if (!fread(&chunkid, 2, 1, f) ||
				!fread(&chunklen, 4, 1, f))
				return (feof(f) != 0);
			if (need_swap) {
				trimesh::swap_ushort(chunkid);
				trimesh::swap_unsigned(chunklen);
			}
			//dprintf("Found chunk %x of length %d\n", chunkid, chunklen);
			switch (chunkid) {
			case CHUNK_3DS_MAIN:
			case CHUNK_3DS_MODEL:
				// Just recurse into this chunk
				break;

			case CHUNK_3DS_OBJ:
				// Skip name, then recurse
				while (!feof(f) && fgetc(f))
					;
				break;

			case CHUNK_3DS_MESH:
				mstart = model->vertices.size();
				break;

			case CHUNK_3DS_VERT: {
				unsigned short nverts;
				if (!fread(&nverts, 2, 1, f))
				{
					if (tracer)
						tracer->failed("fread failed");
					return false;
				}
				if (need_swap)
					trimesh::swap_ushort(nverts);
				read_verts_bin(f, model, need_swap,
					nverts, 12, 0, -1, -1, false, -1);
				break;
			}
			case CHUNK_3DS_FACE: {
				unsigned short nfaces;
				if (!fread(&nfaces, 2, 1, f))
				{
					if (tracer)
						tracer->failed("fread failed");
					return false;
				}
				if (need_swap)
					trimesh::swap_ushort(nfaces);
				//dprintf("\n  Reading %d faces... ", nfaces);
				int old_nfaces = model->faces.size();
				int nfacets = old_nfaces + nfaces;

				int calltime = nfacets / 10;
				if (calltime <= 0)
					calltime = nfacets;
				model->faces.resize(nfacets);
				for (int i = old_nfaces; i < nfacets; i++) {

					if (tracer && i % calltime == 1)
					{
						tracer->progress((float)i / (float)nfacets);
					}
					if (tracer && tracer->interrupt())
						return false;

					unsigned short buf[4];
					COND_READ(true, buf[0], 8);
					if (need_swap) {
						trimesh::swap_ushort(buf[0]);
						trimesh::swap_ushort(buf[1]);
						trimesh::swap_ushort(buf[2]);
					}
					model->faces[i][0] = mstart + buf[0];
					model->faces[i][1] = mstart + buf[1];
					model->faces[i][2] = mstart + buf[2];
				}
				break;
			}
			default: {
				// Skip over this chunk
				fseek(f, chunklen - 6, SEEK_CUR);
			}
			}
		}

		if (tracer)
		{
			tracer->success();
		}
		return true;
	}

}