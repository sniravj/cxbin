#include "cxbinmanager.h"
#include "cxbin/loaderimpl.h"
#include "trimesh2/TriMesh.h"

namespace cxbin
{
	CXBinManager cxmanager;
	CXBinManager::CXBinManager()
	{
		registerLoaderImpl(&m_stlLoader);
		registerLoaderImpl(&m_plyLoader);
	}

	CXBinManager::~CXBinManager()
	{

	}


	void CXBinManager::addLoader(LoaderImpl* impl)
	{
		if (!impl)
			return;

		std::string extension = impl->expectExtension();
		std::map<std::string, LoaderImpl*>::iterator it = m_loaders.find(extension);
		if (it != m_loaders.end())
			return;

		m_loaders.insert(std::pair<std::string, LoaderImpl*>(extension, impl));
	}

	void CXBinManager::removeLoader(LoaderImpl* impl)
	{
		if (!impl)
			return;

		for (std::map<std::string, LoaderImpl*>::iterator it = m_loaders.begin();
			it != m_loaders.end(); ++it)
		{
			if ((*it).second == impl)
			{
				m_loaders.erase(it);
				break;
			}
		}

	}

	std::vector<trimesh::TriMesh*> CXBinManager::load(FILE* f, const std::string& extension,
		ccglobal::Tracer* tracer)
	{
		std::vector<trimesh::TriMesh*> models;
		bool loadSuccess = true;
		do
		{
			unsigned fileSize = 0;
			fseek(f, 0L, SEEK_END);
			fileSize = ftell(f);
			fseek(f, 0L, SEEK_SET);

			LoaderImpl* loader = nullptr;
			if (extension.size() > 0)
			{
				std::map<std::string, LoaderImpl*>::iterator it = m_loaders.find(extension);
				if (it != m_loaders.end() && it->second->tryLoad(f, fileSize))
					loader = it->second;
			}

			if (!loader)
			{
				for (std::map<std::string, LoaderImpl*>::iterator it = m_loaders.begin();
					it != m_loaders.end(); ++it)
				{
					fseek(f, 0L, SEEK_SET);
					if (it->second->tryLoad(f, fileSize))
					{
						loader = it->second;
						break;
					}
				}
			}
			fseek(f, 0L, SEEK_SET);

			if (loader)
				loadSuccess = loader->load(f, fileSize, models, tracer);
		} while (0);
		
		if (!loadSuccess)
		{
			for (trimesh::TriMesh* model : models)
				delete model;
			models.clear();
		}
		return models;
	}

	void registerLoaderImpl(LoaderImpl* impl)
	{
		cxmanager.addLoader(impl);
	}

	void unRegisterLoaderImpl(LoaderImpl* impl)
	{
		cxmanager.removeLoader(impl);
	}
}