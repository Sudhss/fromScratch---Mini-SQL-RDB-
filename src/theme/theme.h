#pragma once

#include <QString>
#include <QColor>
#include <QFont>

namespace minidb {

// ── Theme ─────────────────────────────────────────────────────────────
// Color palette and dark theme constants, similar to Valence's theme.h

namespace theme {

    // ── Background colors ─────────────────────────────────────────────
    inline constexpr auto BG_WINDOW         = "#1e1e2e";   // Main window background
    inline constexpr auto BG_EDITOR         = "#181825";   // Query editor background
    inline constexpr auto BG_SIDEBAR        = "#11111b";   // Schema explorer background
    inline constexpr auto BG_RESULTS        = "#1e1e2e";   // Results table background
    inline constexpr auto BG_TOOLBAR        = "#181825";   // Toolbar background
    inline constexpr auto BG_STATUSBAR      = "#11111b";   // Status bar background
    inline constexpr auto BG_SELECTION      = "#45475a";   // Text selection
    inline constexpr auto BG_CURRENT_LINE   = "#252538";   // Current line highlight
    inline constexpr auto BG_TABLE_ALT      = "#1a1a2e";   // Alternating table row

    // ── Foreground colors ─────────────────────────────────────────────
    inline constexpr auto FG_PRIMARY        = "#cdd6f4";   // Primary text
    inline constexpr auto FG_SECONDARY      = "#a6adc8";   // Secondary / dimmed text
    inline constexpr auto FG_MUTED          = "#6c7086";   // Muted text (line numbers)
    inline constexpr auto FG_ACCENT         = "#89b4fa";   // Accent color (links, highlights)

    // ── Syntax highlighting ───────────────────────────────────────────
    inline constexpr auto SYN_KEYWORD       = "#89b4fa";   // SQL keywords (SELECT, FROM, etc.)
    inline constexpr auto SYN_FUNCTION      = "#cba6f7";   // Functions (COUNT, SUM, etc.)
    inline constexpr auto SYN_STRING        = "#a6e3a1";   // String literals
    inline constexpr auto SYN_NUMBER        = "#fab387";   // Numeric literals
    inline constexpr auto SYN_OPERATOR      = "#89dceb";   // Operators (=, <, >, etc.)
    inline constexpr auto SYN_COMMENT       = "#6c7086";   // Comments
    inline constexpr auto SYN_TYPE          = "#f9e2af";   // Data types (INT, VARCHAR, etc.)
    inline constexpr auto SYN_TABLE         = "#f38ba8";   // Table names (context-dependent)
    inline constexpr auto SYN_NULL          = "#f38ba8";   // NULL, TRUE, FALSE

    // ── Accent / UI colors ────────────────────────────────────────────
    inline constexpr auto ACCENT_PRIMARY    = "#89b4fa";   // Primary accent (buttons, active tabs)
    inline constexpr auto ACCENT_SUCCESS    = "#a6e3a1";   // Success state
    inline constexpr auto ACCENT_WARNING    = "#f9e2af";   // Warning state
    inline constexpr auto ACCENT_ERROR      = "#f38ba8";   // Error state
    inline constexpr auto ACCENT_INFO       = "#89dceb";   // Info state

    // ── Border colors ─────────────────────────────────────────────────
    inline constexpr auto BORDER_SUBTLE     = "#313244";   // Subtle borders
    inline constexpr auto BORDER_ACTIVE     = "#89b4fa";   // Active/focused borders

    // ── Font configuration ────────────────────────────────────────────
    inline constexpr auto FONT_MONO         = "Cascadia Code";  // Monospace font
    inline constexpr auto FONT_UI           = "Segoe UI";       // UI font
    inline constexpr int  FONT_SIZE_EDITOR  = 13;
    inline constexpr int  FONT_SIZE_UI      = 10;
    inline constexpr int  FONT_SIZE_STATUS  = 9;

    // ── Helper to build the application-wide dark theme stylesheet ────
    inline QString darkStyleSheet() {
        return QString(R"(
            QMainWindow {
                background-color: %1;
            }
            QMenuBar {
                background-color: %2;
                color: %3;
                border-bottom: 1px solid %4;
                padding: 2px;
            }
            QMenuBar::item:selected {
                background-color: %5;
                border-radius: 4px;
            }
            QMenu {
                background-color: %2;
                color: %3;
                border: 1px solid %4;
                border-radius: 6px;
                padding: 4px;
            }
            QMenu::item:selected {
                background-color: %5;
                border-radius: 4px;
            }
            QToolBar {
                background-color: %2;
                border-bottom: 1px solid %4;
                spacing: 4px;
                padding: 2px 8px;
            }
            QToolButton {
                color: %3;
                background: transparent;
                border: none;
                border-radius: 4px;
                padding: 6px 12px;
                font-size: 12px;
            }
            QToolButton:hover {
                background-color: %5;
            }
            QToolButton:pressed {
                background-color: %6;
            }
            QSplitter::handle {
                background-color: %4;
                width: 1px;
                height: 1px;
            }
            QTreeWidget {
                background-color: %7;
                color: %3;
                border: none;
                font-size: 12px;
                outline: none;
            }
            QTreeWidget::item {
                padding: 4px 8px;
                border-radius: 4px;
            }
            QTreeWidget::item:selected {
                background-color: %5;
                color: %8;
            }
            QTreeWidget::item:hover {
                background-color: %9;
            }
            QHeaderView::section {
                background-color: %2;
                color: %3;
                border: none;
                border-right: 1px solid %4;
                border-bottom: 1px solid %4;
                padding: 6px 8px;
                font-weight: bold;
                font-size: 11px;
            }
            QTableView {
                background-color: %1;
                color: %3;
                border: none;
                gridline-color: %4;
                selection-background-color: %5;
                alternate-background-color: %10;
                font-size: 12px;
            }
            QStatusBar {
                background-color: %7;
                color: %11;
                border-top: 1px solid %4;
                font-size: 11px;
                padding: 2px 8px;
            }
            QScrollBar:vertical {
                background: transparent;
                width: 10px;
                margin: 0px;
            }
            QScrollBar::handle:vertical {
                background: %4;
                border-radius: 5px;
                min-height: 30px;
            }
            QScrollBar::handle:vertical:hover {
                background: %12;
            }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
                height: 0px;
            }
            QScrollBar:horizontal {
                background: transparent;
                height: 10px;
                margin: 0px;
            }
            QScrollBar::handle:horizontal {
                background: %4;
                border-radius: 5px;
                min-width: 30px;
            }
            QScrollBar::handle:horizontal:hover {
                background: %12;
            }
            QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
                width: 0px;
            }
        )")
        .arg(BG_WINDOW)           // %1
        .arg(BG_TOOLBAR)          // %2
        .arg(FG_PRIMARY)          // %3
        .arg(BORDER_SUBTLE)       // %4
        .arg(BG_SELECTION)        // %5
        .arg(ACCENT_PRIMARY)      // %6
        .arg(BG_SIDEBAR)          // %7
        .arg(ACCENT_PRIMARY)      // %8
        .arg(BG_CURRENT_LINE)     // %9
        .arg(BG_TABLE_ALT)        // %10
        .arg(FG_SECONDARY)        // %11
        .arg(FG_MUTED);           // %12
    }

} // namespace theme
} // namespace minidb
