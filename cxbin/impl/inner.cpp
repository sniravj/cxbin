#include "inner.h"

#include "stringutil/filenameutil.h"
#include "trimesh2/endianutil.h"
#include "cxbin/convert.h"

#define BIGNUM 1.0e10f
namespace cxbin
{
	// Read a bunch of vertices from an ASCII file.
	// Parameters are as in read_verts_bin, but offsets are in
	// (white-space-separated) words, rather than in bytes
	bool read_verts_asc(FILE* f, trimesh::TriMesh* mesh,
		int nverts, int vert_len, int vert_pos, int vert_norm,
		int vert_color, bool float_color, int vert_conf)
	{
		if (nverts < 0 || vert_len < 3 || vert_pos < 0)
			return false;
		if (nverts == 0)
			return true;

		int old_nverts = mesh->vertices.size();
		int new_nverts = old_nverts + nverts;
		mesh->vertices.resize(new_nverts);
		if (vert_norm > 0)
			mesh->normals.resize(new_nverts);
		if (vert_color > 0)
			mesh->colors.resize(new_nverts);
		if (vert_conf > 0)
			mesh->confidences.resize(new_nverts);

		char buf[1024];
		stringutil::skip_comments(f);
		//dprintf("\n  Reading %d vertices... ", nverts);
		for (int i = old_nverts; i < new_nverts; i++) {
			for (int j = 0; j < vert_len; j++) {
				if (j == vert_pos) {
					if (fscanf(f, "%f %f %f",
						&mesh->vertices[i][0],
						&mesh->vertices[i][1],
						&mesh->vertices[i][2]) != 3)
						return false;
					j += 2;
				}
				else if (j == vert_norm) {
					if (fscanf(f, "%f %f %f",
						&mesh->normals[i][0],
						&mesh->normals[i][1],
						&mesh->normals[i][2]) != 3)
						return false;
					j += 2;
				}
				else if (j == vert_color && float_color) {
					float r, g, b;
					if (fscanf(f, "%f %f %f", &r, &g, &b) != 3)
						return false;
					mesh->colors[i] = trimesh::Color(r, g, b);
					j += 2;
				}
				else if (j == vert_color && !float_color) {
					int r, g, b;
					if (fscanf(f, "%d %d %d", &r, &g, &b) != 3)
						return false;
					mesh->colors[i] = trimesh::Color(r, g, b);
					j += 2;
				}
				else if (j == vert_conf) {
					if (fscanf(f, "%f", &mesh->confidences[i]) != 1)
						return false;
				}
				else {
					GET_WORD();
				}
			}
		}

		return true;
	}



	// Optimized reader for the simple case of just vertices w/o other properties
	bool slurp_verts_bin(FILE* f, trimesh::TriMesh* mesh, bool need_swap, int nverts)
	{
		int first = mesh->vertices.size() - nverts + 1;
		COND_READ(true, mesh->vertices[first][0], (nverts - 1) * 12);
		if (need_swap) {
			for (size_t i = first; i < mesh->vertices.size(); i++) {
				trimesh::swap_float(mesh->vertices[i][0]);
				trimesh::swap_float(mesh->vertices[i][1]);
				trimesh::swap_float(mesh->vertices[i][2]);
			}
		}
		return true;
	}

	// Figure out whether the need_swap setting makes sense, or whether this
	// file incorrectly declares its endianness
	void check_need_swap(const trimesh::point& p, bool& need_swap)
	{
		float p0 = p[0], p1 = p[1], p2 = p[2];
		if (need_swap) {
			trimesh::swap_float(p0);
			trimesh::swap_float(p1);
			trimesh::swap_float(p2);
		}
		bool makes_sense = (p0 > -BIGNUM && p0 < BIGNUM &&
			p1 > -BIGNUM && p1 < BIGNUM &&
			p2 > -BIGNUM && p2 < BIGNUM);
		if (makes_sense)
			return;

		trimesh::swap_float(p0);
		trimesh::swap_float(p1);
		trimesh::swap_float(p2);

		bool makes_sense_swapped = (p0 > -BIGNUM && p0 < BIGNUM &&
			p1 > -BIGNUM && p1 < BIGNUM &&
			p2 > -BIGNUM && p2 < BIGNUM);
		if (makes_sense_swapped) {
			//dprintf("Compensating for bogus endianness...\n");
			need_swap = !need_swap;
		}
	}

	// Read nverts vertices from a binary file.
	// vert_len = total length of a vertex record in bytes
	// vert_pos, vert_norm, vert_color, vert_conf =
	//   position of vertex coordinates / normals / color / confidence in record
	// need_swap = swap for opposite endianness
	// float_color = colors are 4-byte float * 3, vs 1-byte uchar * 3
	bool read_verts_bin(FILE* f, trimesh::TriMesh* mesh, bool& need_swap,
		int nverts, int vert_len, int vert_pos, int vert_norm,
		int vert_color, bool float_color, int vert_conf)
	{
		const int vert_size = 12;
		const int norm_size = 12;
		const int color_size = float_color ? 12 : 3;
		const int conf_size = 4;

		if (nverts < 0 || vert_len < 12 || vert_pos < 0)
			return false;
		if (nverts == 0)
			return true;

		int old_nverts = mesh->vertices.size();
		int new_nverts = old_nverts + nverts;
		mesh->vertices.resize(new_nverts);

		bool have_norm = (vert_norm >= 0);
		bool have_color = (vert_color >= 0);
		bool have_conf = (vert_conf >= 0);
		if (have_norm)
			mesh->normals.resize(new_nverts);
		if (have_color)
			mesh->colors.resize(new_nverts);
		if (have_conf)
			mesh->confidences.resize(new_nverts);

		unsigned char* buf = new unsigned char[vert_len];
		COND_READ(true, buf[0], vert_len);

		int i = old_nverts;
		memcpy(&mesh->vertices[i][0], &buf[vert_pos], vert_size);
		if (have_norm)
			memcpy(&mesh->normals[i][0], &buf[vert_norm], norm_size);
		if (have_color && float_color)
			memcpy(&mesh->colors[i][0], &buf[vert_color], color_size);
		if (have_color && !float_color)
			mesh->colors[i] = trimesh::Color(&buf[vert_color]);
		if (have_conf)
			memcpy(&mesh->confidences[i], &buf[vert_conf], conf_size);

		check_need_swap(mesh->vertices[i], need_swap);
		if (need_swap) {
			trimesh::swap_float(mesh->vertices[i][0]);
			trimesh::swap_float(mesh->vertices[i][1]);
			trimesh::swap_float(mesh->vertices[i][2]);
			if (have_norm) {
				trimesh::swap_float(mesh->normals[i][0]);
				trimesh::swap_float(mesh->normals[i][1]);
				trimesh::swap_float(mesh->normals[i][2]);
			}
			if (have_color && float_color) {
				trimesh::swap_float(mesh->colors[i][0]);
				trimesh::swap_float(mesh->colors[i][1]);
				trimesh::swap_float(mesh->colors[i][2]);
			}
			if (have_conf)
				trimesh::swap_float(mesh->confidences[i]);
		}

		//dprintf("\n  Reading %d vertices... ", nverts);
		if (vert_len == 12 && sizeof(trimesh::point) == 12 && nverts > 1)
			return slurp_verts_bin(f, mesh, need_swap, nverts);
		while (++i < new_nverts) {
			COND_READ(true, buf[0], vert_len);
			memcpy(&mesh->vertices[i][0], &buf[vert_pos], vert_size);
			if (have_norm)
				memcpy(&mesh->normals[i][0], &buf[vert_norm], norm_size);
			if (have_color && float_color)
				memcpy(&mesh->colors[i][0], &buf[vert_color], color_size);
			if (have_color && !float_color)
				mesh->colors[i] = trimesh::Color(&buf[vert_color]);
			if (have_conf)
				memcpy(&mesh->confidences[i], &buf[vert_conf], conf_size);

			if (need_swap) {
				trimesh::swap_float(mesh->vertices[i][0]);
				trimesh::swap_float(mesh->vertices[i][1]);
				trimesh::swap_float(mesh->vertices[i][2]);
				if (have_norm) {
					trimesh::swap_float(mesh->normals[i][0]);
					trimesh::swap_float(mesh->normals[i][1]);
					trimesh::swap_float(mesh->normals[i][2]);
				}
				if (have_color && float_color) {
					trimesh::swap_float(mesh->colors[i][0]);
					trimesh::swap_float(mesh->colors[i][1]);
					trimesh::swap_float(mesh->colors[i][2]);
				}
				if (have_conf)
					trimesh::swap_float(mesh->confidences[i]);
			}
		}

		return true;
	}

	// Read range grid data from a binary file
	bool read_grid_bin(FILE* f, trimesh::TriMesh* mesh, bool need_swap)
	{
		//dprintf("\n  Reading range grid... ");
		int ngrid = mesh->grid_width * mesh->grid_height;
		mesh->grid.resize(ngrid, trimesh::TriMesh::GRID_INVALID);
		for (int i = 0; i < ngrid; i++) {
			int n = fgetc(f);
			if (n == EOF)
				return false;
			while (n--) {
				if (!fread((void*) & (mesh->grid[i]), 4, 1, f))
					return false;
				if (need_swap)
					trimesh::swap_int(mesh->grid[i]);
			}
		}

		//mesh->triangulate_grid();
		return true;
	}


	// Read range grid data from an ASCII file
	bool read_grid_asc(FILE* f, trimesh::TriMesh* mesh)
	{
		//dprintf("\n  Reading range grid... ");
		int ngrid = mesh->grid_width * mesh->grid_height;
		mesh->grid.resize(ngrid, trimesh::TriMesh::GRID_INVALID);
		for (int i = 0; i < ngrid; i++) {
			int n;
			if (fscanf(f, "%d", &n) != 1)
				return false;
			while (n--) {
				if (fscanf(f, "%d", &(mesh->grid[i])) != 1)
					return false;
			}
		}

		//mesh->triangulate_grid();
		return true;
	}


	// Read triangle strips from a binary file
	bool read_strips_bin(FILE* f, trimesh::TriMesh* mesh, bool need_swap)
	{
		int striplen;
		COND_READ(true, striplen, 4);
		if (need_swap)
			trimesh::swap_int(striplen);

		int old_striplen = mesh->tstrips.size();
		int new_striplen = old_striplen + striplen;
		mesh->tstrips.resize(new_striplen);

		//dprintf("\n  Reading triangle strips... ");
		COND_READ(true, mesh->tstrips[old_striplen], 4 * striplen);
		if (need_swap) {
			for (int i = old_striplen; i < new_striplen; i++)
				trimesh::swap_int(mesh->tstrips[i]);
		}

		return true;
	}


	// Read triangle strips from an ASCII file
	bool read_strips_asc(FILE* f, trimesh::TriMesh* mesh)
	{
		stringutil::skip_comments(f);
		int striplen;
		if (fscanf(f, "%d", &striplen) != 1)
			return false;
		int old_striplen = mesh->tstrips.size();
		int new_striplen = old_striplen + striplen;
		mesh->tstrips.resize(new_striplen);

		//dprintf("\n  Reading triangle strips... ");
		stringutil::skip_comments(f);
		for (int i = old_striplen; i < new_striplen; i++)
			if (fscanf(f, "%d", &mesh->tstrips[i]) != 1)
				return false;

		return true;
	}


	// Read nfaces faces from a binary file.
	// face_len = total length of face record, *not counting the indices*
	//  (Yes, this is bizarre, but there is potentially a variable # of indices...)
	// face_count = offset within record of the count of indices in this face
	//  (If this is -1, does not read a count and assumes triangles)
	// face_idx = offset within record of the indices themselves
	bool read_faces_bin(FILE* f, trimesh::TriMesh* mesh, bool need_swap,
		int nfaces, int face_len, int face_count, int face_idx)
	{
		if (nfaces < 0 || face_idx < 0)
			return false;

		if (nfaces == 0)
			return true;

		//dprintf("\n  Reading %d faces... ", nfaces);

		int old_nfaces = mesh->faces.size();
		int new_nfaces = old_nfaces + nfaces;
		mesh->faces.reserve(new_nfaces);

		// face_len doesn't include the indices themeselves, since that's
		// potentially variable-length
		int face_skip = face_len - face_idx;

		std::vector<unsigned char> buf(std::max(face_idx, face_skip));
		std::vector<int> thisface;
		for (int i = 0; i < nfaces; i++) {
			COND_READ(face_idx > 0, buf[0], face_idx);

			unsigned this_ninds = 3;
			if (face_count >= 0) {
				// Read count - either 1 or 4 bytes
				if (face_idx - face_count == 4) {
					this_ninds = *(unsigned*) & (buf[face_count]);
					if (need_swap)
						trimesh::swap_unsigned(this_ninds);
				}
				else {
					this_ninds = buf[face_count];
				}
			}
			thisface.resize(this_ninds);
			COND_READ(true, thisface[0], 4 * this_ninds);
			if (need_swap) {
				for (size_t j = 0; j < thisface.size(); j++)
					trimesh::swap_int(thisface[j]);
			}
			tess(mesh->vertices, thisface, mesh->faces);
			COND_READ(face_skip > 0, buf[0], face_skip);
		}

		return true;
	}


	// Read a bunch of faces from an ASCII file
	bool read_faces_asc(FILE* f, trimesh::TriMesh* mesh, int nfaces,
		int face_len, int face_count, int face_idx,bool read_to_eol /* = false */)
	{
		if (nfaces < 0 || face_idx < 0)
			return false;

		if (nfaces == 0)
			return true;

		int old_nfaces = mesh->faces.size();
		int new_nfaces = old_nfaces + nfaces;
		mesh->faces.reserve(new_nfaces);

		char buf[1024];
		stringutil::skip_comments(f);
		//dprintf("\n  Reading %d faces... ", nfaces);
		std::vector<int> thisface;
		for (int i = 0; i < nfaces; i++) {
			thisface.clear();
			int this_face_count = 3;
			for (int j = 0; j < face_len + this_face_count; j++) {
				if (j >= face_idx && j < face_idx + this_face_count) {
					thisface.push_back(0);
					if (!fscanf(f, " %d", &(thisface.back()))) {
						//dprintf("Couldn't read vertex index %d for face %d\n",
						//	j - face_idx, i);
						return false;
					}
				}
				else if (j == face_count) {
					if (!fscanf(f, " %d", &this_face_count)) {
						//dprintf("Couldn't read vertex count for face %d\n", i);
						return false;
					}
				}
				else {
					GET_WORD();
				}
			}
			tess(mesh->vertices, thisface, mesh->faces);
			if (read_to_eol) {
				while (1) {
					int c = fgetc(f);
					if (c == EOF || c == '\n')
						break;
				}
			}
		}

		return true;
	}

	// Read nfaces faces from a binary file.
// face_len = total length of face record, *not counting the indices*
//  (Yes, this is bizarre, but there is potentially a variable # of indices...)
// face_count = offset within record of the count of indices in this face
//  (If this is -1, does not read a count and assumes triangles)
// face_idx = offset within record of the indices themselves
	bool read_faces_bin_with_color(FILE* f, trimesh::TriMesh* mesh, bool need_swap,
		int nfaces, int face_len, int face_count, int face_idx,
		int face_color, bool face_float_color)
	{
		if (nfaces < 0 || face_idx < 0)
			return false;

		if (nfaces == 0)
			return true;

		//dprintf("\n  Reading %d faces... ", nfaces);

		int old_nfaces = mesh->faces.size();
		int new_nfaces = old_nfaces + nfaces;
		mesh->faces.reserve(new_nfaces);

		if (face_color > 0)
			mesh->colors.resize(new_nfaces);

		// face_len doesn't include the indices themeselves, since that's
		// potentially variable-length
		int face_skip = face_len - face_idx;
		if (face_color)
			face_color -= face_idx;

		std::vector<unsigned char> buf(std::max(face_idx, face_skip));
		std::vector<int> thisface;
		for (int i = 0; i < nfaces; i++) {
			COND_READ(face_idx > 0, buf[0], face_idx);

			unsigned this_ninds = 3;
			if (face_count >= 0) {
				// Read count - either 1 or 4 bytes
				if (face_idx - face_count == 4) {
					this_ninds = *(unsigned*)&(buf[face_count]);
					if (need_swap)
						trimesh::swap_unsigned(this_ninds);
				}
				else {
					this_ninds = buf[face_count];
				}
			}
			thisface.resize(this_ninds);
			COND_READ(true, thisface[0], 4 * this_ninds);
			if (need_swap) {
				for (size_t j = 0; j < thisface.size(); j++)
					trimesh::swap_int(thisface[j]);
			}
			tess(mesh->vertices, thisface, mesh->faces);

			for (int j = face_idx; j < face_len; j++)
			{
				if (j == face_color && face_float_color) {
					float r, g, b;
					if (fscanf(f, "%f %f %f", &r, &g, &b) != 3)
						return false;
					mesh->colors[i] = trimesh::Color(r, g, b);
					j += 2;
				}
				else if (j == face_color && !face_float_color) {
					unsigned char r, g, b;
					if (fscanf(f, "%c %c %c", &r, &g, &b) != 3)
						return false;
					mesh->colors[i] = trimesh::Color(r, g, b);
					j += 2;
				}
				else {
					COND_READ(face_skip > 0, buf[0], 1);
				}

			}
			//COND_READ(face_skip > 0, buf[0], face_skip);
		}

		return true;
	}


	// Read a bunch of faces from an ASCII file
	bool read_faces_asc_with_color(FILE* f, trimesh::TriMesh* mesh, int nfaces,
		int face_len, int face_count, int face_idx,
		int face_color, bool face_float_color, bool read_to_eol /* = false */)
	{
		if (nfaces < 0 || face_idx < 0)
			return false;

		if (nfaces == 0)
			return true;

		int old_nfaces = mesh->faces.size();
		int new_nfaces = old_nfaces + nfaces;
		mesh->faces.reserve(new_nfaces);

		if (face_color > 0)
			mesh->colors.resize(new_nfaces);

		char buf[1024];
		stringutil::skip_comments(f);
		//dprintf("\n  Reading %d faces... ", nfaces);
		std::vector<int> thisface;
		for (int i = 0; i < nfaces; i++) {
			thisface.clear();
			int this_face_count = 3;
			for (int j = 0; j < face_len + this_face_count; j++) {
				if (j >= face_idx && j < face_idx + this_face_count) {
					thisface.push_back(0);
					if (!fscanf(f, " %d", &(thisface.back()))) {
						//dprintf("Couldn't read vertex index %d for face %d\n",
						//	j - face_idx, i);
						return false;
					}
				}
				else if (j == face_count) {
					if (!fscanf(f, " %d", &this_face_count)) {
						//dprintf("Couldn't read vertex count for face %d\n", i);
						return false;
					}
				}
				else if (j == face_color - 1 + this_face_count)
				{
					if (face_float_color) {
						float r, g, b;
						if (fscanf(f, "%f %f %f", &r, &g, &b) != 3)
							return false;
						mesh->colors[i] = trimesh::Color(r, g, b);
						j += 2;
					}
					else if (!face_float_color) {
						int r, g, b;
						if (fscanf(f, "%d %d %d", &r, &g, &b) != 3)
							return false;
						mesh->colors[i] = trimesh::Color(r, g, b);
						j += 2;
					}
				}
				else {
					GET_WORD();
				}
			}
			tess(mesh->vertices, thisface, mesh->faces);
			if (read_to_eol) {
				while (1) {
					int c = fgetc(f);
					if (c == EOF || c == '\n')
						break;
				}
			}
		}

		return true;
	}
}