/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>

/// <summary>
///     Defines a struct for specifying the color of an RGB LED.
/// </summary>
typedef struct LedColor {
    bool red;
    bool green;
    bool blue;
} LedColor;

/// <summary>
///     Try to get the LedColor by a particular color name.
/// </summary>
/// <param name="colorName">Name of the color.</param>
/// <param name="color">Pointer to receive the LedColor.</param>
/// <returns>
///     true if the color is known (and sets <paramref name="color"/>); false otherwise.
/// </returns>
bool Color_TryGetColorByName(const char *colorName, LedColor *color);

/// <summary>
///     Try to get the name for a particular LedColor
/// </summary>
/// <param name="color">LedColor</param>
/// <param name="colorName">Pointer to receive the name</param>
/// <returns>
///     true if the color is known (and sets <paramref name="colorName"/>); false otherwise.
/// </returns>
bool Color_TryGetNameForColor(const LedColor *color, const char **colorName);
