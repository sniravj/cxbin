__assert_target(trimesh2)

set(SRCS cxbin/interface.h
		 cxbin/load.h
		 cxbin/save.h
		 cxbin/convert.h
		 cxbin/loaderimpl.h
		 cxbin/saverimpl.h
		 cxbin/impl/load.cpp
		 cxbin/impl/save.cpp
		 cxbin/impl/convert.cpp
		 cxbin/impl/cxbinmanager.h
		 cxbin/impl/cxbinmanager.cpp
		 cxbin/impl/inner.h
		 cxbin/impl/inner.cpp
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
		 cxbin/plugin/plugindae.h
		 cxbin/plugin/plugindae.cpp
		 cxbin/plugin/plugin3mf.h
		 cxbin/plugin/plugin3mf.cpp
		 cxbin/cx/writer.h
		 cxbin/cx/writer.cpp
		 cxbin/cx/reader.h
		 cxbin/cx/reader.cpp
		 cxbin/cx/cxutil.h
		 cxbin/cx/cxutil.cpp
		 )
		 
set(LIBS trimesh2 stringutil tinyxml zlib)
set(DEFS)

if(TARGET lib3MF)
	list(APPEND LIBS lib3MF)
	list(APPEND SRCS cxbin/plugin/plugin3mf.h
					 cxbin/plugin/plugin3mf.cpp
					 )
endif()


if(BUILD_CXBIN_STATIC)
	__add_real_target(cxbin lib SOURCE ${SRCS}
								LIB ${LIBS}
								DEF ${DEFS}
								INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
								SOURCE_FOLDER
								)
else()
	list(APPEND DEFS CXBIN_DLL)
	__add_real_target(cxbin dll SOURCE ${SRCS}
								LIB ${LIBS}
								DEF ${DEFS}
								INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
								SOURCE_FOLDER
								)
								
	if(INSTALL_CXBIN)
		INSTALL(TARGETS cxbin
				LIBRARY DESTINATION lib
				ARCHIVE DESTINATION .
				RUNTIME DESTINATION bin)
	endif()
endif()

