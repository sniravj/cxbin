//
//  analysis.h
//  iOSDemo4OSG
//
//

#ifndef CXBIN_ANALYSIS_H
#define CXBIN_ANALYSIS_H

#include <vector>
#include <string>
#include <memory>
#include "cxbin/interface.h"

namespace ccglobal
{
    class Tracer;
}

namespace cxbin
{
    class AssociateFileInfo;

    CXBIN_API std::vector<std::pair<std::string, bool>> getMeshFileList(const std::string& dirName, ccglobal::Tracer* tracer);
    CXBIN_API void getAssociateFileList(const std::string& filePath, ccglobal::Tracer* tracer, std::vector<std::shared_ptr<cxbin::AssociateFileInfo>> &out);
}


#endif /* CXBIN_ANALYSIS_H */
