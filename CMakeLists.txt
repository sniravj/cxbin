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
		 )
		 
set(LIBS trimesh2 stringutil)
set(DEFS)

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
endif()

