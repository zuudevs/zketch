cmake_minimum_required(VERSION 4.3)
project(
	zketch
	VERSION 1.0
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

# Hybrid toolchain: Clang/LLVM frontend + MSVC ABI/linker/Windows SDK (clang-cl)
# CMake detects this as both MSVC=ON and CMAKE_CXX_COMPILER_ID=Clang
# Use MSVC-style flags (/) since clang-cl uses MSVC frontend syntax
if(MSVC)
    add_compile_options(
        /W4
        /permissive-
        /Zc:__cplusplus
        /EHsc
        /utf-8
    )
    # Clang-specific extras (accepted by clang-cl)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(
            /clang:-fcolor-diagnostics
            /clang:-Wno-unused-command-line-argument
        )
    else()
        # Pure MSVC fallback
        add_compile_options(/MP)
    endif()
else()
    # Non-MSVC Clang (Linux/macOS, not expected for this project)
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -fexceptions
    )
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

# Core header-only library (metric + util layers)
add_library(zketch_core INTERFACE)
target_include_directories(zketch_core INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)

add_library(zketch_core_impl STATIC
    ${CMAKE_SOURCE_DIR}/src/core/application.cpp
    ${CMAKE_SOURCE_DIR}/src/core/main_thread_queue.cpp
    ${CMAKE_SOURCE_DIR}/src/core/message_loop.cpp
    ${CMAKE_SOURCE_DIR}/src/log/logger.cpp
)
target_include_directories(zketch_core_impl PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(zketch_core_impl PUBLIC zketch_core)

add_library(zketch_ui STATIC
    ${CMAKE_SOURCE_DIR}/src/ui/label.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/button.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/window.cpp
    ${CMAKE_SOURCE_DIR}/src/event/event_dispatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/log/logger.cpp
)
target_include_directories(zketch_ui PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(zketch_ui PUBLIC zketch_core)

# Renderer library — Direct2D + DirectWrite backend
add_library(zketch_render STATIC
    ${CMAKE_SOURCE_DIR}/src/render/renderer.cpp
)
target_include_directories(zketch_render PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(zketch_render PUBLIC zketch_core zketch_ui d2d1 dwrite windowscodecs)

# IO library — Widget serializer and parser
add_library(zketch_io STATIC
    ${CMAKE_SOURCE_DIR}/src/io/widget_serializer.cpp
    ${CMAKE_SOURCE_DIR}/src/io/widget_parser.cpp
)
target_include_directories(zketch_io PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(zketch_io PUBLIC zketch_core zketch_ui)

add_executable(${PROJECT_NAME}
    ${CMAKE_SOURCE_DIR}/src/main.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE zketch_core zketch_core_impl zketch_ui zketch_render)

# Testing
enable_testing()

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

add_executable(test_error
    ${CMAKE_SOURCE_DIR}/test/error/test_error.cpp
)
target_link_libraries(test_error PRIVATE zketch_core Catch2::Catch2WithMain)
target_include_directories(test_error PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_logger
    ${CMAKE_SOURCE_DIR}/test/log/test_logger.cpp
    ${CMAKE_SOURCE_DIR}/src/log/logger.cpp
)
target_link_libraries(test_logger PRIVATE zketch_core Catch2::Catch2WithMain)
target_include_directories(test_logger PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_pos
    ${CMAKE_SOURCE_DIR}/test/metric/test_pos.cpp
)
target_link_libraries(test_pos PRIVATE zketch_core Catch2::Catch2WithMain)
target_include_directories(test_pos PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_size
    ${CMAKE_SOURCE_DIR}/test/metric/test_size.cpp
)
target_link_libraries(test_size PRIVATE zketch_core Catch2::Catch2WithMain)
target_include_directories(test_size PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_arithmetic_clamp
    ${CMAKE_SOURCE_DIR}/test/metric/test_arithmetic_clamp.cpp
)
target_link_libraries(test_arithmetic_clamp PRIVATE zketch_core Catch2::Catch2WithMain)
target_include_directories(test_arithmetic_clamp PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_native_wrapper
    ${CMAKE_SOURCE_DIR}/test/util/test_native_wrapper.cpp
)
target_link_libraries(test_native_wrapper PRIVATE zketch_core Catch2::Catch2WithMain)
target_include_directories(test_native_wrapper PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_event_dispatcher
    ${CMAKE_SOURCE_DIR}/test/event/test_event_dispatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/event/event_dispatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/log/logger.cpp
)
target_link_libraries(test_event_dispatcher PRIVATE zketch_core Catch2::Catch2WithMain)
target_include_directories(test_event_dispatcher PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_label
    ${CMAKE_SOURCE_DIR}/test/ui/test_label.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/label.cpp
)
target_link_libraries(test_label PRIVATE zketch_core zketch_render Catch2::Catch2WithMain d2d1 dwrite)
target_include_directories(test_label PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_button
    ${CMAKE_SOURCE_DIR}/test/ui/test_button.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/button.cpp
)
target_link_libraries(test_button PRIVATE zketch_core zketch_render Catch2::Catch2WithMain d2d1 dwrite)
target_include_directories(test_button PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_panel
    ${CMAKE_SOURCE_DIR}/test/ui/test_panel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/label.cpp
)
target_link_libraries(test_panel PRIVATE zketch_core zketch_render Catch2::Catch2WithMain d2d1 dwrite)
target_include_directories(test_panel PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_renderer
    ${CMAKE_SOURCE_DIR}/test/render/test_renderer.cpp
    ${CMAKE_SOURCE_DIR}/src/render/renderer.cpp
    ${CMAKE_SOURCE_DIR}/src/log/logger.cpp
)
target_link_libraries(test_renderer PRIVATE zketch_core zketch_ui Catch2::Catch2WithMain d2d1 dwrite)
target_include_directories(test_renderer PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_window
    ${CMAKE_SOURCE_DIR}/test/ui/test_window.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/window.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/label.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/button.cpp
    ${CMAKE_SOURCE_DIR}/src/event/event_dispatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/log/logger.cpp
    ${CMAKE_SOURCE_DIR}/src/render/renderer.cpp
)
target_link_libraries(test_window PRIVATE zketch_core Catch2::Catch2WithMain d2d1 dwrite)
target_include_directories(test_window PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_message_loop
    ${CMAKE_SOURCE_DIR}/test/core/test_message_loop.cpp
    ${CMAKE_SOURCE_DIR}/src/core/main_thread_queue.cpp
    ${CMAKE_SOURCE_DIR}/src/core/message_loop.cpp
    ${CMAKE_SOURCE_DIR}/src/log/logger.cpp
)
target_link_libraries(test_message_loop PRIVATE zketch_core Catch2::Catch2WithMain)
target_include_directories(test_message_loop PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_main_thread_queue
    ${CMAKE_SOURCE_DIR}/test/core/test_main_thread_queue.cpp
    ${CMAKE_SOURCE_DIR}/src/core/main_thread_queue.cpp
    ${CMAKE_SOURCE_DIR}/src/log/logger.cpp
)
target_link_libraries(test_main_thread_queue PRIVATE zketch_core Catch2::Catch2WithMain)
target_include_directories(test_main_thread_queue PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_widget_io
    ${CMAKE_SOURCE_DIR}/test/io/test_widget_io.cpp
    ${CMAKE_SOURCE_DIR}/src/io/widget_serializer.cpp
    ${CMAKE_SOURCE_DIR}/src/io/widget_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/window.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/label.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/button.cpp
    ${CMAKE_SOURCE_DIR}/src/event/event_dispatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/log/logger.cpp
    ${CMAKE_SOURCE_DIR}/src/render/renderer.cpp
)
target_link_libraries(test_widget_io PRIVATE zketch_core Catch2::Catch2WithMain d2d1 dwrite)
target_include_directories(test_widget_io PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_widget_parser_errors
    ${CMAKE_SOURCE_DIR}/test/io/test_widget_parser_errors.cpp
    ${CMAKE_SOURCE_DIR}/src/io/widget_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/window.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/label.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/button.cpp
    ${CMAKE_SOURCE_DIR}/src/event/event_dispatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/log/logger.cpp
    ${CMAKE_SOURCE_DIR}/src/render/renderer.cpp
)
target_link_libraries(test_widget_parser_errors PRIVATE zketch_core Catch2::Catch2WithMain d2d1 dwrite)
target_include_directories(test_widget_parser_errors PRIVATE ${CMAKE_SOURCE_DIR}/include)

# ---------------------------------------------------------------------------
# Examples
# ---------------------------------------------------------------------------

add_executable(example_hello_window
    ${CMAKE_SOURCE_DIR}/example/hello_window/main.cpp
)
target_link_libraries(example_hello_window PRIVATE
    zketch_core_impl zketch_ui zketch_render
)
set_target_properties(example_hello_window PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

add_executable(example_game_loop
    ${CMAKE_SOURCE_DIR}/example/game_loop/main.cpp
)
target_link_libraries(example_game_loop PRIVATE
    zketch_core_impl zketch_ui zketch_render
)
set_target_properties(example_game_loop PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

include(Catch)
catch_discover_tests(test_error)
catch_discover_tests(test_message_loop)
catch_discover_tests(test_main_thread_queue)
catch_discover_tests(test_logger)
catch_discover_tests(test_pos)
catch_discover_tests(test_size)
catch_discover_tests(test_arithmetic_clamp)
catch_discover_tests(test_native_wrapper)
catch_discover_tests(test_event_dispatcher)
catch_discover_tests(test_label)
catch_discover_tests(test_button)
catch_discover_tests(test_panel)
catch_discover_tests(test_renderer)
catch_discover_tests(test_window)
catch_discover_tests(test_widget_io)
catch_discover_tests(test_widget_parser_errors)