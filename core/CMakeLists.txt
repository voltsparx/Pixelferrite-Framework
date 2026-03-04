add_library(pixelferrite_core
    src/analysis_engine.cpp
    src/banner.cpp
    src/config_loader.cpp
    src/colors.cpp
    src/console.cpp
    src/detection_engine.cpp
    src/encoder_engine.cpp
    src/explain_engine.cpp
    src/evasion_engine.cpp
    src/image_engine.cpp
    src/logger.cpp
    src/module_loader.cpp
    src/nop_engine.cpp
    src/path_verifier.cpp
    src/session_manager.cpp
    src/simulation_engine.cpp
)

target_include_directories(pixelferrite_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(pixelferrite_core PRIVATE /W4)
else()
    target_compile_options(pixelferrite_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
