cmake_minimum_required(VERSION 3.16)
project(MiniSQLRDB VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# ── Source files by component ──────────────────────────────────────────

set(CORE_SOURCES
    src/core/types.h        src/core/types.cpp
    src/core/errors.h       src/core/errors.cpp
    src/core/database.h     src/core/database.cpp
)

set(STORAGE_SOURCES
    src/storage/page.h      src/storage/page.cpp
    src/storage/pager.h     src/storage/pager.cpp
    src/storage/record.h    src/storage/record.cpp
    src/storage/table.h     src/storage/table.cpp
    src/storage/catalog.h   src/storage/catalog.cpp
    src/storage/btree.h     src/storage/btree.cpp
)

set(SQL_SOURCES
    src/sql/token.h         src/sql/token.cpp
    src/sql/lexer.h         src/sql/lexer.cpp
    src/sql/ast.h           src/sql/ast.cpp
    src/sql/parser.h        src/sql/parser.cpp
    src/sql/result.h        src/sql/result.cpp
)

set(ENGINE_SOURCES
    src/engine/evaluator.h  src/engine/evaluator.cpp
    src/engine/planner.h    src/engine/planner.cpp
    src/engine/executor.h   src/engine/executor.cpp
)

set(UI_SOURCES
    src/ui/main_window.h        src/ui/main_window.cpp
    src/ui/query_editor.h       src/ui/query_editor.cpp
    src/ui/results_table.h      src/ui/results_table.cpp
    src/ui/schema_explorer.h    src/ui/schema_explorer.cpp
    src/ui/status_bar.h         src/ui/status_bar.cpp
)

set(THEME_SOURCES
    src/theme/theme.h
    src/theme/sql_highlighter.h src/theme/sql_highlighter.cpp
)

# ── Executable ─────────────────────────────────────────────────────────

add_executable(${PROJECT_NAME}
    src/main.cpp
    ${CORE_SOURCES}
    ${STORAGE_SOURCES}
    ${SQL_SOURCES}
    ${ENGINE_SOURCES}
    ${UI_SOURCES}
    ${THEME_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# ── Platform-specific ──────────────────────────────────────────────────

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    # Allow console output for CLI mode (MSVC only)
    if(MSVC)
        target_link_options(${PROJECT_NAME} PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
    endif()
endif()

# ── Source Groups (IDE organization) ───────────────────────────────────

source_group("Core"     FILES ${CORE_SOURCES})
source_group("Storage"  FILES ${STORAGE_SOURCES})
source_group("SQL"      FILES ${SQL_SOURCES})
source_group("Engine"   FILES ${ENGINE_SOURCES})
source_group("UI"       FILES ${UI_SOURCES})
source_group("Theme"    FILES ${THEME_SOURCES})
