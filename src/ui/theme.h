#pragma once
#include <QString>

namespace minidb {

class Theme {
public:
    static constexpr const char* BG_MAIN = "#1E1E1E";
    static constexpr const char* BG_SIDEBAR = "#252526";
    static constexpr const char* BG_EDITOR = "#1E1E1E";
    static constexpr const char* BG_TOOLBAR = "#333333";
    static constexpr const char* BG_MENU = "#252526";
    static constexpr const char* BG_HOVER = "#2A2D2E";
    
    static constexpr const char* FG_PRIMARY = "#D4D4D4";
    static constexpr const char* FG_MUTED = "#858585";
    
    static constexpr const char* BORDER_COLOR = "#3C3C3C";
    
    static constexpr const char* SYN_KEYWORD = "#569CD6";
    static constexpr const char* SYN_STRING = "#CE9178";
    static constexpr const char* SYN_NUMBER = "#B5CEA8";
    static constexpr const char* SYN_COMMENT = "#6A9955";
    static constexpr const char* SYN_TYPE = "#4EC9B0";
    static constexpr const char* SYN_FUNCTION = "#DCDCAA";
    static constexpr const char* SYN_NULL = "#569CD6";
    static constexpr const char* SYN_OPERATOR = "#D4D4D4";

    static QString darkStyleSheet() {
        return QString(
            "QMainWindow { background-color: %1; color: %2; }"
            "QWidget { font-family: 'Segoe UI', Arial, sans-serif; }"
            "QSplitter::handle { background-color: %3; }"
            "QScrollBar:vertical { background-color: %1; width: 14px; margin: 0px; }"
            "QScrollBar::handle:vertical { background-color: %4; min-height: 20px; border-radius: 7px; margin: 2px; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        ).arg(BG_MAIN).arg(FG_PRIMARY).arg(BORDER_COLOR).arg(BG_HOVER);
    }
};

} // namespace minidb
