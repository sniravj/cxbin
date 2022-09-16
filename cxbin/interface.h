#ifndef CXBIN_INTERFACE_1604911737496_H
#define CXBIN_INTERFACE_1604911737496_H
#include "ccglobal/export.h"
#include <string>

#if USE_CXBIN_DLL
	#define CXBIN_API CC_DECLARE_IMPORT
#elif USE_CXBIN_STATIC
	#define CXBIN_API CC_DECLARE_STATIC
#else
	#if CXBIN_DLL
		#define CXBIN_API CC_DECLARE_EXPORT
	#else
		#define CXBIN_API CC_DECLARE_STATIC
	#endif
#endif


namespace cxbin
{
    enum class CXBinLoaderCode : int {
        no_error              = 0,
        file_invalid          = 1000,
        file_not_exist        = 1001,
        file_mtl_not_exist    = 1002,
        file_pic_not_exist    = 1003,
    };

    class AssociateFileInfo
    {
    public:
        std::string path;
        CXBinLoaderCode code;
        std::string msg;
    };
}

#endif // CXBIN_INTERFACE_1604911737496_H
