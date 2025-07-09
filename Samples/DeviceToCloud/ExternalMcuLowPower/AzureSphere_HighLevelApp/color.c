/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdlib.h>
#include <string.h>

#include "color.h"

typedef struct NamedColor {
    const char *name;
    LedColor color;
} NamedColor;

/// <summary>
/// Defines the mappings of color names to LedColor values. Note that the names must match the
/// values defined for the "Color" value of "NextFlavour" property in the Azure IoT Central app.
/// </summary>
static NamedColor availableColors[] = {
    {.name = "black", .color = {.red = false, .green = false, .blue = false}},
    {.name = "red", .color = {.red = true, .green = false, .blue = false}},
    {.name = "green", .color = {.red = false, .green = true, .blue = false}},
    {.name = "blue", .color = {.red = false, .green = false, .blue = true}},
    {.name = "yellow", .color = {.red = true, .green = true, .blue = false}},
    {.name = "cyan", .color = {.red = false, .green = true, .blue = true}},
    {.name = "magenta", .color = {.red = true, .green = false, .blue = true}},
    {.name = "white", .color = {.red = true, .green = true, .blue = true}}};

static const size_t numColors = sizeof(availableColors) / sizeof(NamedColor);

bool Color_TryGetColorByName(const char *colorName, LedColor *color)
{
    if (colorName == NULL) {
        return false;
    }

    for (int i = 0; i < numColors; i++) {
        if (strcmp(availableColors[i].name, colorName) == 0) {
            *color = availableColors[i].color;
            return true;
        }
    }

    return false;
}

bool Color_TryGetNameForColor(const LedColor *color, const char **colorName)
{
    if (color == NULL) {
        return false;
    }

    for (int i = 0; i < numColors; i++) {
        if (availableColors[i].color.red == color->red &&
            availableColors[i].color.green == color->green &&
            availableColors[i].color.blue == color->blue) {
            *colorName = availableColors[i].name;
            return true;
        }
    }

    return false;
}
