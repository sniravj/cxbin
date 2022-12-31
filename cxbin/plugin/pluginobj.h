#ifndef CXBIN_PLUGINOBJ_1630741230194_H
#define CXBIN_PLUGINOBJ_1630741230194_H
#include "cxbin/loaderimpl.h"
#include "cxbin/saverimpl.h"

namespace trimesh {
	struct Material;
}

#define MAX_OBJ_READLINE_LEN 10240
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
		void associateFileList(FILE* f, ccglobal::Tracer* tracer, const std::string& filePath, std::vector<std::shared_ptr<AssociateFileInfo>>& out) override;

    private:
        bool loadMtl(const std::string& fileName, std::vector<trimesh::Material>& out);
		bool loadMap(trimesh::TriMesh* mesh);
		int checkMtlCompleteness(std::string filename, ccglobal::Tracer* tracer, std::vector<std::shared_ptr<AssociateFileInfo>>& out);
        
        void removeRepeatMaterial(trimesh::TriMesh* mesh);
    private:
	};


	class ObjSaver : public SaverImpl
	{
	public:
		ObjSaver();
		virtual ~ObjSaver();

		std::string expectExtension() override;
		bool save(FILE* f, trimesh::TriMesh* out, ccglobal::Tracer* tracer, std::string fileName = "") override;
	private:
	};
}

#endif // CXBIN_PLUGINOBJ_1630741230194_H
