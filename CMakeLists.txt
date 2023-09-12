__cc_find(trimesh2)
__assert_target(trimesh2)

__cc_find(zlib_static)
__assert_target(zlib_static)

__cc_find(boost_header)

__cc_find(lib3mf_static)

__cc_find(colladadom_static)

__cc_find(freeimage_static)
__assert_target(freeimage_static)

__conan_import(boost dll COMPONENT boost_nowide boost_filesystem)
__assert_target(boost_nowide)
__assert_target(boost_filesystem)

set(SRCS cxbin/interface.h
		 cxbin/load.h
		 cxbin/save.h
		 cxbin/convert.h
		 cxbin/loaderimpl.h
		 cxbin/saverimpl.h
		 cxbin/format.h
		 cxbin/analysis.h
		 cxbin/impl/load.cpp
		 cxbin/impl/save.cpp
		 cxbin/impl/convert.cpp
		 cxbin/impl/cxbinmanager.h
		 cxbin/impl/cxbinmanager.cpp
		 cxbin/impl/inner.h
		 cxbin/impl/inner.cpp
		 cxbin/impl/format.cpp
		 cxbin/impl/analysis.cpp
		 cxbin/impl/loaderimpl.cpp
		 cxbin/impl/util.h
		 cxbin/impl/util.cpp
		 cxbin/impl/imageloader.h
		 cxbin/impl/imageloader.cpp
		 cxbin/impl/freeimageloader.cpp
		 cxbin/impl/optimizeenchaser.h
		 cxbin/impl/optimizeenchaser.cpp
		 cxbin/plugin/pluginstl.h
		 cxbin/plugin/pluginstl.cpp
		 cxbin/plugin/pluginply.h
		 cxbin/plugin/pluginply.cpp
		 cxbin/plugin/pluginobj.h
		 cxbin/plugin/pluginobj.cpp
		 cxbin/plugin/plugincxbin.h
		 cxbin/plugin/plugincxbin.cpp
		 cxbin/plugin/plugin3ds.h
		 cxbin/plugin/plugin3ds.cpp
		 cxbin/plugin/pluginoff.h
		 cxbin/plugin/pluginoff.cpp	
		 cxbin/plugin/pluginwrl.h
		 cxbin/plugin/pluginwrl.cpp
		 cxbin/plugin/pluginoff.cpp
		 cxbin/cx/writer.h
		 cxbin/cx/writer.cpp
		 cxbin/cx/reader.h
		 cxbin/cx/reader.cpp
		 cxbin/cx/cxutil.h
		 cxbin/cx/cxutil.cpp

		 )
		 
set(LIBS zlib_static freeimage_static boost_header
		boost_nowide boost_filesystem
		)
set(ILIBS trimesh2)
set(DEFS BOOST_ALL_NO_LIB HAVE_FREEIMAGE)

if(CC_BC_EMCC)
    list(APPEND DEFS TRIMESH_MAPBUF_RGB)
endif()

if(NOT CC_BC_WIN)
	list(APPEND DEFS __USE_FILE_OFFSET64)
endif()

if(TARGET lib3mf_static)
	list(APPEND LIBS lib3mf_static)
	list(APPEND SRCS cxbin/plugin/plugin3mf.h
					 cxbin/plugin/plugin3mf.cpp
					 )
endif()

if(TARGET colladadom_static)
	list(APPEND SRCS cxbin/plugin/plugindaedom.h
					 cxbin/plugin/plugindaedom.cpp
			)
			
	__cc_find(tinyxml)
	__assert_target(tinyxml)

	list(APPEND LIBS colladadom_static tinyxml pcre_local pcrecpp_local pcreposix_local)
	list(APPEND DEFS DAE_USE_COLLDADA_DOM)
endif()

__add_common_library(cxbin)

if(CC_INSTALL_CXBIN)
	__install_directory_specif(${CMAKE_CURRENT_SOURCE_DIR}/cxbin cxbin)
endif()
