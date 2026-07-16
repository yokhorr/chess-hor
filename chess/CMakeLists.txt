cmake_minimum_required(VERSION 3.20)
project(minmax_chess)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(chess_lib STATIC
        src/board.cpp
        src/king.cpp
        src/pawn.cpp
        src/piece.cpp
        src/knight.cpp
)

target_include_directories(chess_lib PUBLIC src)

if (EMSCRIPTEN)
    # Сборка WebAssembly-модуля для веб-интерфейса (см. каталог web/ в корне репозитория).
    # Логика движка не дублируется и не изменяется: wasm/api.cpp - это тонкая обвязка,
    # а wasm/bindings.cpp лишь публикует её функции через Embind.
    add_executable(chess_wasm
            wasm/api.cpp
            wasm/bindings.cpp
    )
    target_link_libraries(chess_wasm PRIVATE chess_lib)
    set_target_properties(chess_wasm PROPERTIES OUTPUT_NAME "chess")

    target_link_options(chess_wasm PRIVATE
            "--bind"
            "-sMODULARIZE=1"
            "-sEXPORT_ES6=1"
            "-sEXPORT_NAME=createChessModule"
            "-sENVIRONMENT=web"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sNO_EXIT_RUNTIME=1"
            "-sASSERTIONS=0"
    )
else ()
    add_executable(minmax_chess src/main.cpp)
    target_link_libraries(minmax_chess PRIVATE chess_lib)

    # Тесты подключаются только если в системе есть Catch2 - см. tests/tests.cpp (заглушка).
    find_package(Catch2 3 QUIET)
    if (Catch2_FOUND)
        enable_testing()
        add_executable(tests
                tests/tests.cpp
        )
        target_link_libraries(tests PRIVATE chess_lib Catch2::Catch2WithMain)
    endif ()
endif ()
