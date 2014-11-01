#ifndef _BUTTON_DETAILS_H
#define _BUTTON_DETAILS_H

#include "game/gui/spritebutton.h"
#include "game/gui/text_render.h"

typedef struct {
    spritebutton_click_cb cb;
    const char *text;
    text_direction dir;
    text_halign halign;
    text_valign valign;
    int top;
    int bottom;
    int left;
    int right;
} button_details;

#endif // _BUTTON_DETAILS_H
