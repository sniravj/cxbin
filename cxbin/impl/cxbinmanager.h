#ifndef CXBIN_CXBINMANAGER_1630737422773_H
#define CXBIN_CXBINMANAGER_1630737422773_H
#include <vector>
#include <map>
#include <string>
#include <cstdio>

#include "cxbin/plugin/pluginstl.h"
#include "cxbin/plugin/pluginply.h"
#include "cxbin/plugin/pluginobj.h"
#include "cxbin/plugin/pluginoff.h"
#include "cxbin/plugin/plugin3ds.h"
#include "cxbin/plugin/pluginwrl.h"
#include "cxbin/plugin/plugin3mf.h"
#include "cxbin/plugin/plugindae.h"
#include "cxbin/plugin/plugincxbin.h"

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
		void addSaver(SaverImpl* impl);
		void removeSaver(SaverImpl* impl);

		std::vector<trimesh::TriMesh*> load(FILE* f, const std::string& extension, ccglobal::Tracer* tracer);
		void save(trimesh::TriMesh* mesh, const std::string& fileName, const std::string& extension, ccglobal::Tracer* tracer);
		std::string testFormat(const std::string& fileName);
	protected:
		std::map<std::string, LoaderImpl*> m_loaders;
		std::map<std::string, SaverImpl*> m_savers;

		StlLoader m_stlLoader;
		PlyLoader m_plyLoader;
		ObjLoader m_objLoader;
		OffLoader m_offLoader;
		_3dsLoader m_3dsLoader;
		WrlLoader m_wrlLoader;
		_3mfLoader m_3mfLoader;
		DaeLoader m_daeLoader;
		CXBinLoader m_cxbinLoader;

		CXBinSaver m_cxbinSaver;
		PlySaver m_plySaver;
	};

	extern CXBinManager cxmanager;
}

#endif // CXBIN_CXBINMANAGER_1630737422773_H