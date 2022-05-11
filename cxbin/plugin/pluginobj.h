#ifndef CXBIN_PLUGINOBJ_1630741230194_H
#define CXBIN_PLUGINOBJ_1630741230194_H
#include "cxbin/loaderimpl.h"
#include "trimesh2/TriMesh.h"
namespace cxbin
{
	class ObjLoader : public LoaderImpl
	{
	public:
		ObjLoader();
		virtual ~ObjLoader();

		std::string expectExtension() override;
		bool tryLoad(FILE* f, size_t fileSize) override;
		bool load(FILE* f, size_t fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer) override;
        
    private:
        bool loadMtl(const std::string& fileName, std::vector<trimesh::Material>& out);
    private:
	};
}

#endif // CXBIN_PLUGINOBJ_1630741230194_H
