cmake_minimum_required(VERSION 3.20)
project(CalculatorWithPlugins VERSION 1.0.0 LANGUAGES CXX)

# Строгое требование стандарта C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Исходники и строки в программе — в UTF-8 (консоль переключается на UTF-8 в main.cpp)
if (MSVC)
    add_compile_options(/utf-8)
endif()

# Папка с плагинами по умолчанию
set( calc_PLUGIN_DIR_NAME "plugins" )

# CMake time: генерация конфигурационного заголовочного файла (Generated Sources files на слайде 2)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/config.h"
    @ONLY
)

# Ядро калькулятора (парсер + менеджер плагинов): общее для приложения и примеров
add_library(calc_core STATIC
    src/plugin_manager.cpp
    src/expression_parser.cpp
)

target_include_directories(calc_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

if (NOT WIN32)
    target_link_libraries(calc_core PUBLIC dl)
endif()

# Сборка основного приложения
add_executable(calc src/main.cpp)
target_link_libraries(calc PRIVATE calc_core)

set_target_properties(calc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
)

# Функция для сборки динамических библиотек плагинов (.dll / .so)
function(add_calc_plugin target_name src_file)
    add_library(${target_name} MODULE ${src_file})

    target_include_directories(${target_name} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
    )

    set_target_properties(${target_name} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "${target_name}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/${calc_PLUGIN_DIR_NAME}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/${calc_PLUGIN_DIR_NAME}"
        PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/$<CONFIG>/pdb"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib"
    )

    if (NOT WIN32)
        target_compile_options(${target_name} PRIVATE -fvisibility=hidden)
    endif()

    install(TARGETS ${target_name}
        LIBRARY DESTINATION "${calc_PLUGIN_DIR_NAME}"
        RUNTIME DESTINATION "${calc_PLUGIN_DIR_NAME}"
    )
endfunction()

# Регистрация всех плагинов
add_calc_plugin(funcsin  plugins/funcsin.cpp)
add_calc_plugin(funccos  plugins/funccos.cpp)
add_calc_plugin(funcln   plugins/funcln.cpp)
add_calc_plugin(funcsqrt plugins/funcsqrt.cpp)
add_calc_plugin(funcexp  plugins/funcexp.cpp)
add_calc_plugin(funcdeg  plugins/funcdeg.cpp)


# Install time & CPack time (в соответствии со слайдом 2)
install(TARGETS calc RUNTIME DESTINATION .)
include(CPack)