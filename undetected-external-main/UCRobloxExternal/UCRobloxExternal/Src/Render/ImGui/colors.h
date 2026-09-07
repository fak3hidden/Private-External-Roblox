#pragma once

#include "imgui.h"

namespace Menu
{
    // Mapped from user's original ImVec4 colors
    // 0.26 * 255 = 66, 0.59 * 255 = 150, 0.98 * 255 = 250
    inline ImU32 Accent = IM_COL32(66, 150, 250, 255);
    inline ImU32 DarkAccent = IM_COL32(40, 90, 150, 255);
    
    // 0.08 * 255 = 20
    inline ImU32 Bg = IM_COL32(20, 20, 20, 255);
    inline ImU32 ChildBg = IM_COL32(23, 23, 23, 255);
    inline ImU32 InnerBg = IM_COL32(20, 20, 20, 255);
    
    // 0.18 * 255 = 46
    inline ImU32 Outline = IM_COL32(46, 46, 46, 255);
    
    // 0.90 * 255 = 230
    inline ImU32 Text = IM_COL32(230, 230, 230, 255);
    inline ImU32 TextDim = IM_COL32(128, 140, 140, 255);
}
