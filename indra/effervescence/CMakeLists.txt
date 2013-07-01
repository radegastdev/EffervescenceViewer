# -*- cmake -*-

project(effervescence)

include(00-Common)
include(LLCommon)
include(LLAppearance)
include(LLCharacter)
include(LLImage)
include(LLInventory)
include(LLMath)
include(LLMessage)
include(LLPlugin)
include(LLPrimitive)
include(LLRender)
include(LLUI)
include(LLVFS)
include(LLWINDOW)
include(LLXML)
#include(FreeType)
include(Colladadom)
include(statemachine)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLAPPEARANCE_INCLUDE_DIRS}
    ${LLCHARACTER_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLINVENTORY_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLPLUGIN_INCLUDE_DIRS}
    ${LLPRIMITIVE_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLUI_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    #${FREETYPE_INCLUDE_DIRS}
    ${GLEXT_INCLUDE_DIR}
    ${COLLADADOM_INCLUDE_DIRS}
    ${STATEMACHINE_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/newview/    # viewer
    )

set(effervescence_SOURCE_FILES
    aversion.cpp
    awavefront.cpp
    special_functionality.cpp
    )

set(effervescence_HEADER_FILES
    CMakeLists.txt

    aversion.h
    awavefront.h
    special_functionality.h
    )
        
set_source_files_properties(${effervescence_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND effervescence_SOURCE_FILES ${effervescence_HEADER_FILES})

add_library (effervescence ${effervescence_SOURCE_FILES})
add_dependencies(effervescence prepare)
