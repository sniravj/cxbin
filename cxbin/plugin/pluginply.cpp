#include "pluginply.h"
#include "trimesh2/endianutil.h"
#include "trimesh2/TriMesh.h"
#include "trimesh2/Color.h"

#include "cxbin/convert.h"
#include "stringutil/filenameutil.h"
#include "cxbin/impl/inner.h"
#include "ccglobal/tracer.h"

namespace cxbin
{
	PlySaver::PlySaver()
	{

	}

	PlySaver::~PlySaver()
	{

	}

	std::string PlySaver::expectExtension()
	{
		return "ply";
	}

	// Convert colors float -> uchar
	unsigned char color2uchar(float p)
	{
		return std::min(std::max(int(255.0f * p + 0.5f), 0), 255);
	}

	bool write_ply_header(trimesh::TriMesh* mesh, FILE* f, const char* format,
		bool write_grid, bool write_tstrips,
		bool write_norm, bool float_color)
	{
		FPRINTF(f, "ply\nformat %s 1.0\n", format);
		if (write_grid) {
			FPRINTF(f, "obj_info num_cols %d\n", mesh->grid_width);
			FPRINTF(f, "obj_info num_rows %d\n", mesh->grid_height);
		}
		FPRINTF(f, "element vertex %lu\n",
			(unsigned long)mesh->vertices.size());
		FPRINTF(f, "property float x\n");
		FPRINTF(f, "property float y\n");
		FPRINTF(f, "property float z\n");
		if (write_norm && !mesh->normals.empty()) {
			FPRINTF(f, "property float nx\n");
			FPRINTF(f, "property float ny\n");
			FPRINTF(f, "property float nz\n");
		}
		if (!mesh->colors.empty() && float_color) {
			FPRINTF(f, "property float diffuse_red\n");
			FPRINTF(f, "property float diffuse_green\n");
			FPRINTF(f, "property float diffuse_blue\n");
		}
		if (!mesh->colors.empty() && !float_color) {
			FPRINTF(f, "property uchar diffuse_red\n");
			FPRINTF(f, "property uchar diffuse_green\n");
			FPRINTF(f, "property uchar diffuse_blue\n");
		}
		if (!mesh->confidences.empty()) {
			FPRINTF(f, "property float confidence\n");
		}
		if (write_grid) {
			int ngrid = mesh->grid_width * mesh->grid_height;
			FPRINTF(f, "element range_grid %d\n", ngrid);
			FPRINTF(f, "property list uchar int vertex_indices\n");
		}
		else if (write_tstrips) {
			FPRINTF(f, "element tristrips 1\n");
			FPRINTF(f, "property list int int vertex_indices\n");
		}
		else {
			mesh->need_faces();
			if (!mesh->faces.empty()) {
				FPRINTF(f, "element face %lu\n",
					(unsigned long)mesh->faces.size());
				FPRINTF(f, "property list uchar int vertex_indices\n");
			}
		}
		FPRINTF(f, "end_header\n");
		return true;
	}

	// Helper for write_verts_bin: actually does the writing.
	bool write_verts_bin_helper(trimesh::TriMesh* mesh, FILE* f,
		bool write_norm, bool write_color,
		bool float_color, bool write_conf)
	{
		if ((mesh->normals.empty() || !write_norm) &&
			(mesh->colors.empty() || !write_color) &&
			(mesh->confidences.empty() || !write_conf)) {
			// Optimized vertex-only code
			FWRITE(&(mesh->vertices[0][0]), 12 * mesh->vertices.size(), 1, f);
		}
		else {
			// Generic code
			for (size_t i = 0; i < mesh->vertices.size(); i++) {
				FWRITE(&(mesh->vertices[i][0]), 12, 1, f);
				if (!mesh->normals.empty() && write_norm)
					FWRITE(&(mesh->normals[i][0]), 12, 1, f);
				if (!mesh->colors.empty() && write_color && float_color)
					FWRITE(&(mesh->colors[i][0]), 12, 1, f);
				if (!mesh->colors.empty() && write_color && !float_color) {
					unsigned char c[3] = {
						color2uchar(mesh->colors[i][0]),
						color2uchar(mesh->colors[i][1]),
						color2uchar(mesh->colors[i][2]) };
					FWRITE(&c, 3, 1, f);
				}
				if (!mesh->confidences.empty() && write_conf)
					FWRITE(&(mesh->confidences[i]), 4, 1, f);
			}
		}
		return true;
	}

	// Byte-swap vertex properties
	void swap_vert_props(trimesh::TriMesh* mesh, bool swap_color)
	{
		for (size_t i = 0; i < mesh->vertices.size(); i++) {
			trimesh::swap_float(mesh->vertices[i][0]);
			trimesh::swap_float(mesh->vertices[i][1]);
			trimesh::swap_float(mesh->vertices[i][2]);
		}
		if (!mesh->normals.empty()) {
			for (size_t i = 0; i < mesh->normals.size(); i++) {
				trimesh::swap_float(mesh->normals[i][0]);
				trimesh::swap_float(mesh->normals[i][1]);
				trimesh::swap_float(mesh->normals[i][2]);
			}
		}
		if (!mesh->colors.empty() && swap_color) {
			for (size_t i = 0; i < mesh->normals.size(); i++) {
				trimesh::swap_float(mesh->colors[i][0]);
				trimesh::swap_float(mesh->colors[i][1]);
				trimesh::swap_float(mesh->colors[i][2]);
			}
		}
		if (!mesh->confidences.empty()) {
			for (size_t i = 0; i < mesh->confidences.size(); i++)
				trimesh::swap_float(mesh->confidences[i]);
		}
	}

	// Write a bunch of vertices to a binary file
	bool write_verts_bin(trimesh::TriMesh* mesh, FILE* f, bool need_swap,
		bool write_norm, bool write_color,
		bool float_color, bool write_conf)
	{
		if (need_swap)
			swap_vert_props(mesh, float_color);
		bool ok = write_verts_bin_helper(mesh, f, write_norm, write_color,
			float_color, write_conf);
		if (need_swap)
			swap_vert_props(mesh, float_color);
		return ok;
	}

	// Write tstrips to a binary file
	bool write_strips_bin(trimesh::TriMesh* mesh, FILE* f, bool need_swap)
	{
		if (need_swap) {
			for (size_t i = 0; i < mesh->tstrips.size(); i++)
				trimesh::swap_int(mesh->tstrips[i]);
		}
		bool ok = (fwrite(&(mesh->tstrips[0]), 4 * mesh->tstrips.size(), 1, f) == 1);
		if (need_swap) {
			for (size_t i = 0; i < mesh->tstrips.size(); i++)
				trimesh::swap_int(mesh->tstrips[i]);
		}
		return ok;
	}

	// Write range grid to a binary file
	bool write_grid_bin(trimesh::TriMesh* mesh, FILE* f, bool need_swap)
	{
		unsigned char zero = 0;
		unsigned char one = 1;
		for (size_t i = 0; i < mesh->grid.size(); i++) {
			if (mesh->grid[i] < 0) {
				FWRITE(&zero, 1, 1, f);
			}
			else {
				FWRITE(&one, 1, 1, f);
				int g = mesh->grid[i];
				if (need_swap)
					trimesh::swap_int(g);
				FWRITE(&g, 4, 1, f);
			}
		}
		return true;
	}

	// Write a bunch of faces to a binary file
	bool write_faces_bin(trimesh::TriMesh* mesh, FILE* f, bool need_swap,
		int before_face_len, const char* before_face,
		int after_face_len, const char* after_face)
	{
		mesh->need_faces();
		if (need_swap) {
			for (size_t i = 0; i < mesh->faces.size(); i++) {
				trimesh::swap_int(mesh->faces[i][0]);
				trimesh::swap_int(mesh->faces[i][1]);
				trimesh::swap_int(mesh->faces[i][2]);
			}
		}
		bool ok = true;
		for (size_t i = 0; i < mesh->faces.size(); i++) {
			if (before_face_len) {
				if (fwrite(before_face, before_face_len, 1, f) != 1) {
					ok = false;
					goto out;
				}
			}
			if (fwrite(&(mesh->faces[i][0]), 12, 1, f) != 1) {
				ok = false;
				goto out;
			}
			if (after_face_len) {
				if (fwrite(after_face, after_face_len, 1, f) != 1) {
					ok = false;
					goto out;
				}
			}
		}
	out:
		if (need_swap) {
			for (size_t i = 0; i < mesh->faces.size(); i++) {
				trimesh::swap_int(mesh->faces[i][0]);
				trimesh::swap_int(mesh->faces[i][1]);
				trimesh::swap_int(mesh->faces[i][2]);
			}
		}
		return ok;
	}

	bool write_ply_binary(trimesh::TriMesh* mesh, FILE* f,
		bool little_endian, bool write_norm, bool write_grid, bool float_color)
	{
		// Write a binary ply file
		if (write_norm)
			mesh->need_normals();
		
		bool write_tstrips = !write_grid && !mesh->tstrips.empty();
		bool need_swap = little_endian ^ trimesh::we_are_little_endian();
		
		const char* format = little_endian ?
			"binary_little_endian" : "binary_big_endian";
		if (!write_ply_header(mesh, f, format, write_grid, write_tstrips,
			write_norm, float_color))
			return false;
		if (!write_verts_bin(mesh, f, need_swap, write_norm, true,
			float_color, true))
			return false;
		if (write_grid) {
			return write_grid_bin(mesh, f, need_swap);
		}
		else if (write_tstrips) {
			int s = mesh->tstrips.size();
			if (need_swap)
				trimesh::swap_int(s);
			FWRITE(&s, 4, 1, f);
			mesh->convert_strips(trimesh::TriMesh::TSTRIP_TERM);
			bool ok = write_strips_bin(mesh, f, need_swap);
			mesh->convert_strips(trimesh::TriMesh::TSTRIP_LENGTH);
			return ok;
		}
		// else write faces
		char buf[1] = { 3 };
		return write_faces_bin(mesh, f, need_swap, 1, buf, 0, 0);
	}

	bool PlySaver::save(FILE* f, trimesh::TriMesh* out, ccglobal::Tracer* tracer, std::string fileName)
	{
		return write_ply_binary(out, f, trimesh::we_are_little_endian() ? true : false, 
			false, false, false);
	}

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

	bool PlyLoader::tryLoad(FILE* f, size_t fileSize)
	{
		bool binary = false;
		bool need_swap = false;
		return parsePlyFormat(f, binary, need_swap);
	}

	bool PlyLoader::load(FILE* f, size_t fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
	{
		char buf[1024];
		bool binary = false, need_swap = false, float_color = false;
		int result, nverts = 0, nfaces = 0, nstrips = 0, ngrid = 0;
		int vert_len = 0, vert_pos = -1, vert_norm = -1;
		int vert_color = -1, vert_conf = -1;
		int face_len = 0, face_count = -1, face_idx = -1;
		int face_color = -1, face_float_color = false;

		if (!parsePlyFormat(f, binary, need_swap))
		{
			if (tracer)
				tracer->failed("Format failed");
			return false;
		}

		trimesh::TriMesh* model = new trimesh::TriMesh();
		out.push_back(model);

		// Skip until we find vertices
		int skip1 = 0;
		while (!LINE_IS("end_header") && !LINE_IS("element vertex")) {
			if (tracer && tracer->interrupt())
				return false;

			char elem_name[1024];
			int nelem = 0, elem_len = 0;
			sscanf(buf, "element %s %d", elem_name, &nelem);
			GET_LINE();
			while (LINE_IS("property")) {
				if (!ply_property(buf, elem_len, binary))
				{
					if (tracer)
						tracer->failed("Format failed");
					return false;
				}
				GET_LINE();
			}
			skip1 += nelem * elem_len;
		}

		// Find number of vertices
		result = sscanf(buf, "element vertex %d\n", &nverts);
		if (result != 1) {
			if (tracer)
				tracer->failed("vertex failed");
			return false;
		}

		// Parse vertex properties
		GET_LINE();
		while (LINE_IS("property")) {
			if (tracer && tracer->interrupt())
				return false;

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
			{
				if (tracer)
					tracer->failed("property failed");
				return false;
			}

			GET_LINE();
		}

		// Skip until we find faces
		int skip2 = 0;
		while (!LINE_IS("end_header") && !LINE_IS("element face") &&
			!LINE_IS("element tristrips") && !LINE_IS("element range_grid")) {
			if (tracer && tracer->interrupt())
				return false;

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
			if (tracer && tracer->interrupt())
				return false;

			if (sscanf(buf, "element face %d\n", &nfaces) != 1)
			{
				if (tracer)
					tracer->failed("nfaces failed");
				return false;
			}
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
				{
					if (tracer)
						tracer->failed("property failed");
					return false;
				}

				if (LINE_IS("property uchar diffuse_red") ||
					LINE_IS("property uint8 diffuse_red") ||
					LINE_IS("property uchar red") ||
					LINE_IS("property uint8 red"))
					face_color = face_len;
				if (LINE_IS("property float diffuse_red") ||
					LINE_IS("property float32 diffuse_red") ||
					LINE_IS("property float red") ||
					LINE_IS("property float32 red"))
					face_color = face_len, face_float_color = true;

				GET_LINE();
			}
		}
		else if (LINE_IS("element tristrips")) {
			nstrips = 1;
			GET_LINE();
			if (!LINE_IS("property list int int vertex_ind") &&
				!LINE_IS("property list int32 int32 vertex_ind"))
			{
				if (tracer)
					tracer->failed("property list failed");
				return false;
			}
			GET_LINE();
		}
		else if (LINE_IS("element range_grid")) {
			if (sscanf(buf, "element range_grid %d\n", &ngrid) != 1)
			{
				if (tracer)
					tracer->failed("element range_grid failed");
				return false;
			}
			if (ngrid != model->grid_width * model->grid_height) {
				if (tracer)
					tracer->failed("ngrid failed");
				return false;
			}
			GET_LINE();
			if (!LINE_IS("property list uchar int vertex_ind") &&
				!LINE_IS("property list uint8 int32 vertex_ind") &&
				!LINE_IS("property list char int vertex_ind") &&
				!LINE_IS("property list int8 int32 vertex_ind"))
			{
				if (tracer)
					tracer->failed("property list unit failed");
				return false;
			}
			GET_LINE();
		}

		while (LINE_IS("property")) {
			if (!ply_property(buf, face_len, binary))
			{
				if (tracer)
					tracer->failed("property failed");
				return false;
			}
			GET_LINE();
		}

		if (tracer)
		{
			tracer->progress(0.1f);
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
			{
				if (tracer)
					tracer->failed("read verts bin failed");
				return false;
			}
		}
		else {
			if (!read_verts_asc(f, model, nverts, vert_len,
				vert_pos, vert_norm, vert_color,
				float_color, vert_conf))
			{
				if (tracer)
					tracer->failed("read verts asc failed");
				return false;
			}
		}

		if (tracer)
		{
			tracer->progress(0.5f);
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
				{
					if (tracer)
						tracer->failed("read grid bin failed");
					return false;
				}
			}
			else {
				if (!read_grid_asc(f, model))
				{
					if (tracer)
						tracer->failed("read grid asc failed");
					return false;
				}
			}
		}
		else if (nstrips) {
			if (binary) {
				if (!read_strips_bin(f, model, need_swap))
				{
					if (tracer)
						tracer->failed("read strips bin failed");
					return false;
				}
			}
			else {
				if (!read_strips_asc(f, model))
				{
					if (tracer)
						tracer->failed("read strips asc failed");
					return false;
				}
			}
			model->convert_strips(trimesh::TriMesh::TSTRIP_LENGTH);
		}
		else if (nfaces) {
			if (binary) {
				if (!read_faces_bin_with_color(f, model, need_swap, nfaces,
					face_len, face_count, face_idx,
					face_color, face_float_color))
				{
					if (tracer)
						tracer->failed("read faces bin failed");
					return false;
				}
			}
			else {
				if (!read_faces_asc_with_color(f, model, nfaces,
					face_len, face_count, face_idx, face_color, face_float_color, false))
				{
					if (tracer)
						tracer->failed("read faces asc failed");
					return false;
				}
			}
		}
		if (tracer)
		{
			tracer->progress(0.9f);
			tracer->success();
		}
		return true;
	}
}