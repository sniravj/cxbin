#include "cxbinmanager.h"
#include "cxbin/loaderimpl.h"
#include "cxbin/convert.h"
#include "stringutil/filenameutil.h"

#include "trimesh2/TriMesh.h"

#include "ccglobal/tracer.h"
#include "ccglobal/log.h"
#include "ccglobal/platform.h"
#include "mmesh/trimesh/trimeshutil.h"

namespace cxbin
{
	size_t getFileSize(FILE* file)
	{
		if (!file)
			return 0;

		size_t size = 0;
		fseek(file, 0L, SEEK_END);
		size = _cc_ftelli64(file);
		fseek(file, 0L, SEEK_SET);

		LOGI("CXBinManager open file size [%zu]", size);
		return size;
	}

	CXBinManager cxmanager;
	CXBinManager::CXBinManager()
	{
		registerLoaderImpl(&m_stlLoader);
		registerLoaderImpl(&m_plyLoader);
		registerLoaderImpl(&m_objLoader);
#ifndef __WEB__
		registerLoaderImpl(&m_offLoader);
		registerLoaderImpl(&m_3dsLoader);
		registerLoaderImpl(&m_wrlLoader);
		registerLoaderImpl(&m_3mfLoader);
		registerLoaderImpl(&m_daeLoader);
#endif
		registerLoaderImpl(&m_cxbinLoader);

		registerSaverImpl(&m_cxbinSaver);
		registerSaverImpl(&m_plySaver);
		registerSaverImpl(&m_stlSaver);
		registerSaverImpl(&m_objSaver);
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

	void CXBinManager::addSaver(SaverImpl* impl)
	{
		if (!impl)
			return;

		std::string extension = impl->expectExtension();
		std::map<std::string, SaverImpl*>::iterator it = m_savers.find(extension);
		if (it != m_savers.end())
			return;

		m_savers.insert(std::pair<std::string, SaverImpl*>(extension, impl));
	}

	void CXBinManager::removeSaver(SaverImpl* impl)
	{
		if (!impl)
			return;

		for (std::map<std::string, SaverImpl*>::iterator it = m_savers.begin();
			it != m_savers.end(); ++it)
		{
			if ((*it).second == impl)
			{
				m_savers.erase(it);
				break;
			}
		}
	}

	std::string CXBinManager::testFormat(const std::string& fileName)
	{
		FILE* f = fopen(fileName.c_str(), "rb");
		std::string extension = stringutil::extensionFromFileName(fileName, true);

		if(!f)
		{
			LOGE("CXBinManager Open File [%s] Failed.", fileName.c_str());
			return "";
		}

		size_t fileSize = getFileSize(f);

		LoaderImpl* loader = nullptr;
		if (extension.size() > 0)
		{
			std::map<std::string, LoaderImpl*>::iterator it = m_loaders.find(extension);
			if (it != m_loaders.end() && it->second->tryLoad(f, fileSize))
			{
				loader = it->second;
			}
		}

		if (!loader)
		{
			extension = "";
			for (std::map<std::string, LoaderImpl*>::iterator it = m_loaders.begin();
				 it != m_loaders.end(); ++it)
			{
				fseek(f, 0L, SEEK_SET);
				if (it->second->tryLoad(f, fileSize))
				{
					loader = it->second;
					extension = it->first;
					break;
				}
			}
		}

		fclose(f);
		return extension;
	}

    void CXBinManager::associateFileList(const std::string& fileName, ccglobal::Tracer* tracer, std::vector<std::shared_ptr<AssociateFileInfo>>& out)
    {
        FILE* f = fopen(fileName.c_str(), "rb");
        if(!f)
        {
            LOGE("CXBinManager Open File [%s] Failed.", fileName.c_str());
            
            auto err = new cxbin::AssociateFileInfo();
            err->code = CXBinLoaderCode::file_not_exist;
            err->path = fileName;
            err->msg = "file not exist";
            
            std::shared_ptr<AssociateFileInfo> info;
            info.reset(err);
            out.push_back(info);
            return;
        }
        
        std::string extension = stringutil::extensionFromFileName(fileName, true);
        size_t fileSize = getFileSize(f);

        LoaderImpl* loader = nullptr;
        if (extension.size() > 0)
        {
            std::map<std::string, LoaderImpl*>::iterator it = m_loaders.find(extension);
            if (it != m_loaders.end() && it->second->tryLoad(f, fileSize))
            {
                loader = it->second;
            }
        }

        if (!loader)
        {
            extension = "";
            for (std::map<std::string, LoaderImpl*>::iterator it = m_loaders.begin();
                 it != m_loaders.end(); ++it)
            {
                fseek(f, 0L, SEEK_SET);
                if (it->second->tryLoad(f, fileSize))
                {
                    loader = it->second;
                    extension = it->first;
                    break;
                }
            }
        }

        if (loader) {
            fseek(f, 0L, SEEK_SET);
            loader->associateFileList(f, tracer, fileName, out);
        }
        
        fclose(f);
    }

	std::vector<trimesh::TriMesh*> CXBinManager::load(FILE* f, const std::string& extension,
		ccglobal::Tracer* tracer, const std::string& fileName)
	{
		std::vector<trimesh::TriMesh*> models;
		bool loadSuccess = true;
		do
		{
			size_t fileSize = getFileSize(f);

			std::string use_ext = extension;
			if (use_ext.size() > 0) {
				std::transform(use_ext.begin(), use_ext.end(), use_ext.begin(), tolower);
			}
			
			LoaderImpl* loader = nullptr;
			if (use_ext.size() > 0)
			{
				std::map<std::string, LoaderImpl*>::iterator it = m_loaders.find(use_ext);
				if (it != m_loaders.end() && it->second->tryLoad(f, fileSize))
				{
					loader = it->second;
					formartPrint(tracer, "CXBinManager::load : Use Plugin [%s]", it->first.c_str());
				}
			}

			if (!loader)
			{
				for (std::map<std::string, LoaderImpl*>::iterator it = m_loaders.begin();
					it != m_loaders.end(); ++it)
				{
					formartPrint(tracer, "CXBinManager::load : Try plugin [%s]", it->first.c_str());
					fseek(f, 0L, SEEK_SET);
					if (it->second->tryLoad(f, fileSize))
					{
						loader = it->second;

						formartPrint(tracer, "CXBinManager::load : Searched Plugin [%s]", it->first.c_str());
						break;
					}
				}
			}
			fseek(f, 0L, SEEK_SET);

            if (loader) {
                loader->modelPath = fileName;
				loadSuccess = loader->load(f, fileSize, models, tracer);
                loader->modelPath.erase();
            }
			else if (tracer)
				tracer->failed("CXBinManager::load failed. Can't Find A Plugin.");
		} while (0);
		
		if (loadSuccess)
		{//check
			for (trimesh::TriMesh* model : models)
			{
				int vertexSize = (int)model->vertices.size();
				int faceSize = model->faces.size();
				for (int i = 0; i <faceSize; ++i)
				{
					trimesh::TriMesh::Face& face = model->faces[i];
					if (face.x < 0 || face.x >= vertexSize ||
						face.y < 0 || face.y >= vertexSize ||
						face.z < 0 || face.z >= vertexSize)
					{
						if (tracer)
							tracer->failed("CXBinManager::load . Model FaceIndex is Invalid.");
						loadSuccess = false;
						break;
					}
				}
                mmesh::removeInvalidVertex(model);
                //mmesh::removeNorFaces(model, tracer);
			}
		}

		if (!loadSuccess)
		{
			for (trimesh::TriMesh* model : models)
				delete model;
			models.clear();
		}

		return models;
	}

	bool CXBinManager::save(trimesh::TriMesh* mesh, const std::string& fileName, const std::string& extension, ccglobal::Tracer* tracer)
	{
		if (!mesh && tracer)
		{
			tracer->failed("CXBinManager::save. save empty mesh.");
			return false;
		}

		SaverImpl* saver = nullptr;
		if (extension.size() > 0)
		{
			std::map<std::string, SaverImpl*>::iterator it = m_savers.find(extension);
			if (it != m_savers.end())
			{
				saver = it->second;
				formartPrint(tracer, "CXBinManager::save. use plugin %s", it->first.c_str());
			}
		}

		std::string realFileName = fileName;
		if (!saver)
		{
			saver = &m_cxbinSaver;
			//realFileName += ".";
			//realFileName += "cxbin";

			formartPrint(tracer, "CXBinManager::save. use cxbin extension.");
		}

		FILE* f = fopen(realFileName.c_str(), "wb");

		//formartPrint(tracer, "CXBinManager::save. save file %s.", realFileName.c_str());
		if (!f) {
			if (tracer)
			{
				tracer->failed("CXBinManager::save.  open file failed.");
				return false;
			}
			return false;
		}

		bool success = saver->save(f, mesh, tracer, realFileName.c_str());
		if (success && tracer)
		{
			tracer->progress(1.0f);
			tracer->success();
			success = true;
		}

		fclose(f);
		return success;
	}

	void registerLoaderImpl(LoaderImpl* impl)
	{
		cxmanager.addLoader(impl);
	}

	void unRegisterLoaderImpl(LoaderImpl* impl)
	{
		cxmanager.removeLoader(impl);
	}

	void registerSaverImpl(SaverImpl* impl)
	{
		cxmanager.addSaver(impl);
	}

	void unRegisterSaverImpl(SaverImpl* impl)
	{
		cxmanager.removeSaver(impl);
	}

    
}
