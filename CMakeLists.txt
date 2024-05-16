# CMakeList.txt : CMake project for RmlSolLua, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.13)

project ("RmlSolLua")

set (CMAKE_CXX_STANDARD 17)

find_package (RmlUi REQUIRED)
find_package (sol2 REQUIRED)

# Add source to this project's executable.
add_library (RmlSolLua STATIC)

target_compile_definitions (RmlSolLua
	PRIVATE
		RmlUi_VERSION_MAJOR=${RmlUi_VERSION_MAJOR}
		RmlUi_VERSION_MINOR=${RmlUi_VERSION_MINOR}
)

set(BUILD_SRC_DIR $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
set(BUILD_INCLUDE_DIR $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_sources (RmlSolLua
	PRIVATE
	    "${BUILD_INCLUDE_DIR}/RmlSolLua/RmlSolLua.h"
		"${BUILD_SRC_DIR}/RmlSolLua.cpp"
		"${BUILD_SRC_DIR}/bind/bind.cpp"
		"${BUILD_SRC_DIR}/bind/bind.h"
		"${BUILD_SRC_DIR}/bind/Colour.cpp"
		"${BUILD_SRC_DIR}/bind/Context.cpp"
		"${BUILD_SRC_DIR}/bind/Convert.cpp"
		"${BUILD_SRC_DIR}/bind/DataModel.cpp"
		"${BUILD_SRC_DIR}/bind/Document.cpp"
		"${BUILD_SRC_DIR}/bind/Element.cpp"
		"${BUILD_SRC_DIR}/bind/ElementDerived.cpp"
		"${BUILD_SRC_DIR}/bind/ElementForm.cpp"
		"${BUILD_SRC_DIR}/bind/Event.cpp"
		"${BUILD_SRC_DIR}/bind/Global.cpp"
		"${BUILD_SRC_DIR}/bind/Log.cpp"
		"${BUILD_SRC_DIR}/bind/Vector.cpp"
		"${BUILD_SRC_DIR}/plugin/SolLuaDataModel.cpp"
		"${BUILD_SRC_DIR}/plugin/SolLuaDataModel.h"
		"${BUILD_SRC_DIR}/plugin/SolLuaDocument.cpp"
		"${BUILD_SRC_DIR}/plugin/SolLuaDocument.h"
		"${BUILD_SRC_DIR}/plugin/SolLuaEventListener.cpp"
		"${BUILD_SRC_DIR}/plugin/SolLuaEventListener.h"
		"${BUILD_SRC_DIR}/plugin/SolLuaInstancer.cpp"
		"${BUILD_SRC_DIR}/plugin/SolLuaInstancer.h"
		"${BUILD_SRC_DIR}/plugin/SolLuaPlugin.cpp"
		"${BUILD_SRC_DIR}/plugin/SolLuaPlugin.h"
	PUBLIC
		"${BUILD_INCLUDE_DIR}/RmlSolLua/RmlSolLua.h"
)

target_include_directories (RmlSolLua
	PRIVATE ${BUILD_SRC_DIR}		
	PUBLIC ${BUILD_INCLUDE_DIR}		
)

target_link_libraries (RmlSolLua RmlCore)
if (SOL2_SINGLE)
	target_link_libraries (RmlSolLua sol2::sol2::single)
else ()
	target_link_libraries (RmlSolLua sol2::sol2)
endif()

# TODO: Add tests and install targets if needed.
