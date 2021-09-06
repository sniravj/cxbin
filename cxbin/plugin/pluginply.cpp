#include "pluginply.h"
#include "trimesh2/endianutil.h"
#include "trimesh2/TriMesh.h"

#include "cxbin/convert.h"
#include "stringutil/filenameutil.h"
#include "cxbin/impl/inner.h"

namespace cxbin
{
	PlyLoader::PlyLoader()
	{

	}

	PlyLoader::~PlyLoader()
	{

	}

	std::string PlyLoader::expectExtension()
	{
		return "ply";
	}

	// Return the length in bytes of a ply property type, 0 if can't parse
	int ply_type_len(const char* buf, bool binary)
	{
		using namespace stringutil;

		if (begins_with(buf, "char") ||
			begins_with(buf, "uchar") ||
			begins_with(buf, "int8") ||
			begins_with(buf, "uint8")) {
			return 1;
		}
		else if (begins_with(buf, "short") ||
			begins_with(buf, "ushort") ||
			begins_with(buf, "int16") ||
			begins_with(buf, "uint16")) {
			return (binary ? 2 : 1);
		}
		else if (begins_with(buf, "int") ||
			begins_with(buf, "uint") ||
			begins_with(buf, "float") ||
			begins_with(buf, "int32") ||
			begins_with(buf, "uint32") ||
			begins_with(buf, "float32")) {
			return (binary ? 4 : 1);
		}
		else if (begins_with(buf, "double") ||
			begins_with(buf, "float64")) {
			return (binary ? 8 : 1);
		}
		return 0;
	}

	// Parse a PLY property line, and figure how many bytes it represents
	// Increments "len" by the number of bytes, or by 1 if !binary
	bool ply_property(const char* buf, int& len, bool binary)
	{
		int type_len = ply_type_len(buf + 9, binary);
		if (type_len) {
			len += type_len;
			return true;
		}

		return false;
	}

	bool parsePlyFormat(FILE* f, bool& binary, bool& need_swap)
	{
		binary = false;
		need_swap = false;

		int c = fgetc(f);
		if (c == EOF) {
			return false;
		}

		if (c != 'p')
			return false;

		{
			// See if it's a ply file
			char buf[4];
			if (!fgets(buf, 4, f)) {
				return false;
			}
			if (strncmp(buf, "ly", 2) != 0)
				return false;
		}

		char buf[1024];

		// Read file format
		GET_LINE();
		while (buf[0] && isspace(buf[0]))
			GET_LINE();
		if (LINE_IS("format binary_big_endian 1.0")) {
			binary = true;
			need_swap = trimesh::we_are_little_endian();
		}
		else if (LINE_IS("format binary_little_endian 1.0")) {
			binary = true;
			need_swap = trimesh::we_are_big_endian();
		}
		else if (LINE_IS("format ascii 1.0")) {
			binary = false;
		}
		else {
			return false;
		}

		return true;
	}

	bool PlyLoader::tryLoad(FILE* f, unsigned fileSize)
	{
		bool binary = false;
		bool need_swap = false;
		return parsePlyFormat(f, binary, need_swap);
	}

	bool PlyLoader::load(FILE* f, unsigned fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
	{
		char buf[1024];
		bool binary = false, need_swap = false, float_color = false;
		int result, nverts = 0, nfaces = 0, nstrips = 0, ngrid = 0;
		int vert_len = 0, vert_pos = -1, vert_norm = -1;
		int vert_color = -1, vert_conf = -1;
		int face_len = 0, face_count = -1, face_idx = -1;

		if (parsePlyFormat(f, binary, need_swap))
			return false;

		trimesh::TriMesh* model = new trimesh::TriMesh();
		out.push_back(model);


		// Skip until we find vertices
		int skip1 = 0;
		while (!LINE_IS("end_header") && !LINE_IS("element vertex")) {
			char elem_name[1024];
			int nelem = 0, elem_len = 0;
			sscanf(buf, "element %s %d", elem_name, &nelem);
			GET_LINE();
			while (LINE_IS("property")) {
				if (!ply_property(buf, elem_len, binary))
					return false;
				GET_LINE();
			}
			skip1 += nelem * elem_len;
		}

		// Find number of vertices
		result = sscanf(buf, "element vertex %d\n", &nverts);
		if (result != 1) {
			return false;
		}

		// Parse vertex properties
		GET_LINE();
		while (LINE_IS("property")) {
			if (LINE_IS("property float x") ||
				LINE_IS("property float32 x"))
				vert_pos = vert_len;
			if (LINE_IS("property float nx") ||
				LINE_IS("property float32 nx"))
				vert_norm = vert_len;
			if (LINE_IS("property uchar diffuse_red") ||
				LINE_IS("property uint8 diffuse_red") ||
				LINE_IS("property uchar red") ||
				LINE_IS("property uint8 red"))
				vert_color = vert_len;
			if (LINE_IS("property float diffuse_red") ||
				LINE_IS("property float32 diffuse_red") ||
				LINE_IS("property float red") ||
				LINE_IS("property float32 red"))
				vert_color = vert_len, float_color = true;
			if (LINE_IS("property float confidence") ||
				LINE_IS("property float32 confidence"))
				vert_conf = vert_len;

			if (!ply_property(buf, vert_len, binary))
				return false;

			GET_LINE();
		}

		// Skip until we find faces
		int skip2 = 0;
		while (!LINE_IS("end_header") && !LINE_IS("element face") &&
			!LINE_IS("element tristrips") && !LINE_IS("element range_grid")) {
			char elem_name[1024];
			int nelem = 0, elem_len = 0;
			sscanf(buf, "element %s %d", elem_name, &nelem);
			GET_LINE();
			while (LINE_IS("property")) {
				if (!ply_property(buf, elem_len, binary))
					return false;
				GET_LINE();
			}
			skip2 += nelem * elem_len;
		}


		// Look for faces, tristrips, or range grid
		if (LINE_IS("element face")) {
			if (sscanf(buf, "element face %d\n", &nfaces) != 1)
				return false;
			GET_LINE();
			while (LINE_IS("property")) {
				char count_type[256], ind_type[256];
				if (sscanf(buf, "property list %255s %255s vertex_ind",
					count_type, ind_type) == 2) {
					int count_len = ply_type_len(count_type, binary);
					int ind_len = ply_type_len(ind_type, binary);
					if (count_len && ind_len) {
						face_count = face_len;
						face_idx = face_len + count_len;
						face_len += count_len;
					}
				}
				else if (!ply_property(buf, face_len, binary))
					return false;
				GET_LINE();
			}
		}
		else if (LINE_IS("element tristrips")) {
			nstrips = 1;
			GET_LINE();
			if (!LINE_IS("property list int int vertex_ind") &&
				!LINE_IS("property list int32 int32 vertex_ind"))
				return false;
			GET_LINE();
		}
		else if (LINE_IS("element range_grid")) {
			if (sscanf(buf, "element range_grid %d\n", &ngrid) != 1)
				return false;
			if (ngrid != model->grid_width * model->grid_height) {
				return false;
			}
			GET_LINE();
			if (!LINE_IS("property list uchar int vertex_ind") &&
				!LINE_IS("property list uint8 int32 vertex_ind") &&
				!LINE_IS("property list char int vertex_ind") &&
				!LINE_IS("property list int8 int32 vertex_ind"))
				return false;
			GET_LINE();
		}

		while (LINE_IS("property")) {
			if (!ply_property(buf, face_len, binary))
				return false;
			GET_LINE();
		}

		// Skip to the end of the header
		while (!LINE_IS("end_header"))
			GET_LINE();
		if (binary && buf[10] == '\r') {
			//eprintf("Warning: possibly corrupt file. (Transferred as ASCII instead of BINARY?)\n");
		}

		// Actually read everything in
		if (skip1) {
			if (binary)
				fseek(f, skip1, SEEK_CUR);
			else
				for (int i = 0; i < skip1; i++)
					GET_WORD();
		}
		if (binary) {
			if (!read_verts_bin(f, model, need_swap, nverts, vert_len,
				vert_pos, vert_norm, vert_color,
				float_color, vert_conf))
				return false;
		}
		else {
			if (!read_verts_asc(f, model, nverts, vert_len,
				vert_pos, vert_norm, vert_color,
				float_color, vert_conf))
				return false;
		}

		if (skip2) {
			if (binary)
				fseek(f, skip2, SEEK_CUR);
			else
				for (int i = 0; i < skip2; i++)
					GET_WORD();
		}

		if (ngrid) {
			if (binary) {
				if (!read_grid_bin(f, model, need_swap))
					return false;
			}
			else {
				if (!read_grid_asc(f, model))
					return false;
			}
		}
		else if (nstrips) {
			if (binary) {
				if (!read_strips_bin(f, model, need_swap))
					return false;
			}
			else {
				if (!read_strips_asc(f, model))
					return false;
			}
			model->convert_strips(trimesh::TriMesh::TSTRIP_LENGTH);
		}
		else if (nfaces) {
			if (binary) {
				if (!read_faces_bin(f, model, need_swap, nfaces,
					face_len, face_count, face_idx))
					return false;
			}
			else {
				if (!read_faces_asc(f, model, nfaces,
					face_len, face_count, face_idx, false))
					return false;
			}
		}

		return true;
	}
}