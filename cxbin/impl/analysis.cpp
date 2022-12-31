//
//  analysis.cpp
//  iOSDemo4OSG
//
//

#include <iostream>

#include "cxbin/format.h"
#include "cxbin/analysis.h"
#include "stringutil/filenameutil.h"
#include "ccglobal/tracer.h"
#include "cxbinmanager.h"

#if __ANDROID__
#define _LIBCPP_HAS_NO_OFF_T_FUNCTIONS
#include <boost/filesystem.hpp>
#else
#include <boost/filesystem.hpp>
#endif

namespace cxbin
{
    std::vector<std::pair<std::string, bool>> getMeshFileList(const std::string& dirName, ccglobal::Tracer* tracer)
    {
        std::vector<std::pair<std::string, bool>> list;
        
        boost::filesystem::path path(dirName);
        boost::filesystem::directory_iterator itr(path);
        
        for ( ; itr != boost::filesystem::directory_iterator(); itr++)
        {
            if (tracer && tracer->interrupt()) {
                break;
            }

            if (boost::filesystem::is_regular_file(itr->path())) {
                std::string sub = itr->path().string();
                printf("文件:%s\n", sub.c_str());

                std::string extension = stringutil::extensionFromFileName(sub);
                cxbin::MeshFileFormat extFormat = extension2Format(extension);

                cxbin::MeshFileFormat f = cxbin::testMeshFileFormat(sub);

                if (f != MESH_FILE_FORMAT_UNKOWN) {
                    std::pair<std::string, bool> pair(sub, true);
                    list.push_back(pair);
                } else {
                    if (extFormat != MESH_FILE_FORMAT_UNKOWN) {
                        std::pair<std::string, bool> pair(sub, false);
                        list.push_back(pair);
                    }
                }
            } else if (boost::filesystem::is_directory(itr->path())) {

                std::string sub = itr->path().string();
                std::vector<std::pair<std::string, bool>> subList = getMeshFileList(sub, tracer);
                list.insert(list.end(), subList.begin(), subList.end());
            }
        }
        
        return list;
    }

    void getAssociateFileList(const std::string& filePath, ccglobal::Tracer* tracer, std::vector<std::shared_ptr<AssociateFileInfo>>& out)
    {
        cxmanager.associateFileList(filePath, tracer, out);
    }
}
