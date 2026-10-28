#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ControllerGlyphs.h"

const char *ControllerGlyphs::GetIconPath(eCONTROLLER_BUTTON button, eCONTROLLER_TYPE type)
{
    static const char *dualSenseIcons[g_controllerButtonCount] = {
            "assets/texture/ui/controller/dualsense/x.png",      "assets/texture/ui/controller/dualsense/circle.png",
            "assets/texture/ui/controller/dualsense/square.png", "assets/texture/ui/controller/dualsense/triangle.png",
            "assets/texture/ui/controller/dualsense/start.png",  "assets/texture/ui/controller/dualsense/select.png",
            "assets/texture/ui/controller/dualsense/l1.png",     "assets/texture/ui/controller/dualsense/r1.png",
            "assets/texture/ui/controller/dualsense/l2.png",     "assets/texture/ui/controller/dualsense/r2.png",
            "assets/texture/ui/controller/dualsense/up.png",     "assets/texture/ui/controller/dualsense/down.png",
            "assets/texture/ui/controller/dualsense/left.png",   "assets/texture/ui/controller/dualsense/right.png",
            "assets/texture/ui/controller/dualsense/ls.png",     "assets/texture/ui/controller/dualsense/rs.png"};
    static const char *xboxIcons[g_controllerButtonCount] = {
            "assets/texture/ui/controller/xbox/a.png",    "assets/texture/ui/controller/xbox/b.png",     "assets/texture/ui/controller/xbox/x.png",
            "assets/texture/ui/controller/xbox/y.png",    "assets/texture/ui/controller/xbox/menu.png",  "assets/texture/ui/controller/xbox/view.png",
            "assets/texture/ui/controller/xbox/lb.png",   "assets/texture/ui/controller/xbox/rb.png",    "assets/texture/ui/controller/xbox/lt.png",
            "assets/texture/ui/controller/xbox/rt.png",   "assets/texture/ui/controller/xbox/up.png",    "assets/texture/ui/controller/xbox/down.png",
            "assets/texture/ui/controller/xbox/left.png", "assets/texture/ui/controller/xbox/right.png", "assets/texture/ui/controller/xbox/ls.png",
            "assets/texture/ui/controller/xbox/rs.png"};
    return type == eControllerType_PlayStation ? dualSenseIcons[button] : xboxIcons[button];
}

#endif
