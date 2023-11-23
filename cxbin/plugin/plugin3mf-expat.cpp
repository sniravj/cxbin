#include "plugin3mf-expat.h"
#include <map>

#include "ccglobal/tracer.h"
#include "3mf/3mfimproter.h"

namespace cxbin
{
	_3mfLoader::_3mfLoader()
	{

	}

	_3mfLoader::~_3mfLoader()
	{

	}

	std::string _3mfLoader::expectExtension()
	{
		return "3mf";
	}

	bool _3mfLoader::tryLoad(FILE* , size_t )
	{
		mz_zip_archive archive;
		mz_zip_zero_struct(&archive);

		if (!open_zip_reader(&archive, modelPath)) {
			//add_error("Unable to open the file" + filename);
			return false;
		}

		//use modelPath
		return true;
	}

	bool _3mfLoader::load(FILE*, size_t, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
	{
		//use modelPath
		_BBS_3MF_Importer bbs_3mf_importer;
		std::vector<std::vector<std::string>> colors;
		std::vector<std::vector<std::string>> seams;
		std::vector<std::vector<std::string>> supports;
		bbs_3mf_importer.load3MF(modelPath, out, colors, seams, supports);

		if (tracer)
		{
			tracer->success();
		}
		return true;
	}
}