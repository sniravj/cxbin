//
//  analysis.h
//  iOSDemo4OSG
//
//

#ifndef CXBIN_ANALYSIS_H
#define CXBIN_ANALYSIS_H

#include <vector>
#include <string>

#include "cxbin/interface.h"

namespace ccglobal
{
    class Tracer;
}

namespace cxbin
{
    CXBIN_API std::vector<std::pair<std::string, bool>> getMeshFileList(const std::string& dirName, ccglobal::Tracer* tracer);
    CXBIN_API std::vector<std::string> getAssociateFileList(const std::string& filePath, ccglobal::Tracer* tracer);
}


#endif /* CXBIN_ANALYSIS_H */
