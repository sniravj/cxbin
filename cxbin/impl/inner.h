#ifndef CXBIN_INNER_1630864500772_H
#define CXBIN_INNER_1630864500772_H
#include "trimesh2/TriMesh.h"
#include "ccglobal/tracer.h"

namespace cxbin
{
	bool read_verts_asc(FILE* f, trimesh::TriMesh* mesh,
		int nverts, int vert_len, int vert_pos, int vert_norm,
		int vert_color, bool float_color, int vert_conf);

	bool read_verts_bin(FILE* f, trimesh::TriMesh* mesh, bool& need_swap,
		int nverts, int vert_len, int vert_pos, int vert_norm,
		int vert_color, bool float_color, int vert_conf);

	bool read_grid_bin(FILE* f, trimesh::TriMesh* mesh, bool need_swap);
	bool read_grid_asc(FILE* f, trimesh::TriMesh* mesh);

	bool read_strips_bin(FILE* f, trimesh::TriMesh* mesh, bool need_swap);
	bool read_strips_asc(FILE* f, trimesh::TriMesh* mesh);

	bool read_faces_bin(FILE* f, trimesh::TriMesh* mesh, bool need_swap,
		int nfaces, int face_len, int face_count, int face_idx);
	bool read_faces_asc(FILE* f, trimesh::TriMesh* mesh, int nfaces,
		int face_len, int face_count, int face_idx,bool read_to_eol = false);

	bool read_faces_bin_with_color(FILE* f, trimesh::TriMesh* mesh, bool need_swap,
		int nfaces, int face_len, int face_count, int face_idx,
		int face_color, bool face_float_color);
	bool read_faces_asc_with_color(FILE* f, trimesh::TriMesh* mesh, int nfaces,
		int face_len, int face_count, int face_idx,
		int face_color, bool face_float_color, bool read_to_eol = false);
}

#endif // CXBIN_INNER_1630864500772_H