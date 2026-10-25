cmake_minimum_required(VERSION 3.20)
project(calc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_executable(calc src/main.cpp)


target_include_directories(calc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (NOT WIN32)
    target_link_libraries(calc PRIVATE dl)
endif()


set_target_properties(calc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
)


function(add_calc_plugin name src)
    add_library(${name} MODULE ${src})
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set_target_properties(${name} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "${name}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/plugins"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/plugins"
    )
    if (NOT WIN32)
        target_compile_options(${name} PRIVATE -fvisibility=hidden)
    endif()
endfunction()


add_calc_plugin(funcsin plugins/funcsin.cpp)
add_calc_plugin(funccos plugins/funccos.cpp)
add_calc_plugin(funcln plugins/funcln.cpp)
add_calc_plugin(funcsqrt plugins/funcsqrt.cpp)
add_calc_plugin(funcexp plugins/funcexp.cpp)
add_calc_plugin(funcdeg plugins/funcdeg.cpp)