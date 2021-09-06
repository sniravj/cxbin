#ifndef CXBIN_CXBINMANAGER_1630737422773_H
#define CXBIN_CXBINMANAGER_1630737422773_H
#include <vector>
#include <map>
#include <string>
#include <cstdio>

#include "cxbin/plugin/pluginstl.h"

namespace ccglobal
{
	class Tracer;
}

namespace cxbin
{
	class LoaderImpl;
	class CXBinManager
	{
	public:
		CXBinManager();
		~CXBinManager();

		void addLoader(LoaderImpl* impl);
		void removeLoader(LoaderImpl* impl);

		std::vector<trimesh::TriMesh*> load(FILE* f, const std::string& extension, ccglobal::Tracer* tracer);
	protected:
		std::map<std::string, LoaderImpl*> m_loaders;

		StlLoader m_stlLoader;


	};

	extern CXBinManager cxmanager;
}

#endif // CXBIN_CXBINMANAGER_1630737422773_H