//
//  analysis.cpp
//  iOSDemo4OSG
//
//

#include <iostream>
#include <dirent.h>

#include <cxbin/format.h>
#include "cxbin/analysis.h"
#include "stringutil/filenameutil.h"
#include "ccglobal/tracer.h"
#include "cxbinmanager.h"
namespace cxbin
{
    std::vector<std::pair<std::string, bool>> getMeshFileList(const std::string& dirName, ccglobal::Tracer* tracer)
    {
        std::vector<std::pair<std::string, bool>> list;
        
        DIR *handle = opendir(dirName.c_str());
        if (handle)
        {
            dirent *rc;
            while((rc = readdir(handle)) != NULL)
            {
                if (tracer && tracer->interrupt()) {
                    break;
                }
                char *name = rc->d_name;
                if ( strcmp(name, ".") == 0 || strcmp(name, "..") == 0 )
                    continue;
                
                printf("文件:%s\n", name);
                
                char buffer[512] = {0};
                sprintf(buffer, "%s/%s", dirName.c_str(), name);
                std::string sub = std::string(buffer);
                
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
            }
            
            closedir(handle);
        }
        
        return list;
    }

    std::vector<std::string> getAssociateFileList(const std::string& filePath, ccglobal::Tracer* tracer)
    {
        return cxmanager.associateFileList(filePath, tracer);
    }
}
