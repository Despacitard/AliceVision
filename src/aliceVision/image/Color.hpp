// This file is part of the AliceVision project.
// Copyright (c) 2017 AliceVision contributors.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <string>
#include <cmath>

namespace aliceVision {

class ColorRGBf
{
public:
    float r, g, b;

    inline ColorRGBf()
    {
        r = 0.0;
        g = 0.0;
        b = 0.0;
    }

    inline ColorRGBf(float _r, float _g, float _b)
    {
        r = _r;
        g = _g;
        b = _b;
    }

    inline ColorRGBf& operator=(const ColorRGBf& param)
    {
        r = param.r;
        g = param.g;
        b = param.b;
        return *this;
    }

    inline bool operator==(const ColorRGBf& param)
    {
        return (r == param.r) && (g == param.g) && (b == param.b);
    }

    inline ColorRGBf operator-(const ColorRGBf& _p) const
    {
        return ColorRGBf(r - _p.r, g - _p.g, b - _p.b);
    }

    inline ColorRGBf operator-() const
    {
        return ColorRGBf(-r, -g, -b);
    }

    inline ColorRGBf operator+(const ColorRGBf& _p) const
    {
        return ColorRGBf(r + _p.r, g + _p.g, b + _p.b);
    }

    inline ColorRGBf operator*(const float d) const
    {
        return ColorRGBf(r * d, g * d, b * d);
    }

    inline ColorRGBf operator/(const float d) const
    {
        return ColorRGBf(r / d, g / d, b / d);
    }

    inline ColorRGBf& operator+=(const ColorRGBf& _p)
    {
        r += _p.r;
        g += _p.g;
        b += _p.b;
        return *this;
    }

    inline ColorRGBf& operator-=(const ColorRGBf& _p)
    {
        r -= _p.r;
        g -= _p.g;
        b -= _p.b;
        return *this;
    }

    inline ColorRGBf& operator/=(const int d)
    {
        r /= d;
        g /= d;
        b /= d;
        return *this;
    }
};


class ColorRGBAf
{
public:
    float r, g, b, a;

    inline ColorRGBAf()
    {
        r = 0.0;
        g = 0.0;
        b = 0.0;
        a = 0.0;
    }

    inline ColorRGBAf(float _r, float _g, float _b, float _a)
    {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }

    inline ColorRGBAf& operator=(const ColorRGBAf& param)
    {
        r = param.r;
        g = param.g;
        b = param.b;
        a = param.a;
        return *this;
    }

    inline bool operator==(const ColorRGBAf& param)
    {
        return (r == param.r) && (g == param.g) && (b == param.b) && (a == param.a);
    }

    inline ColorRGBAf operator-(const ColorRGBAf& _p) const
    {
        return ColorRGBAf(r - _p.r, g - _p.g, b - _p.b, a);
    }

    inline ColorRGBAf operator-() const
    {
        return ColorRGBAf(-r, -g, -b, a);
    }

    inline ColorRGBAf operator+(const ColorRGBAf& _p) const
    {
        return ColorRGBAf(r + _p.r, g + _p.g, b + _p.b, a);
    }

    inline ColorRGBAf operator*(const float d) const
    {
        return ColorRGBAf(r * d, g * d, b * d, a);
    }

    inline ColorRGBAf operator/(const float d) const
    {
        return ColorRGBAf(r / d, g / d, b / d, a);
    }

    inline ColorRGBAf& operator+=(const ColorRGBAf& _p)
    {
        r += _p.r;
        g += _p.g;
        b += _p.b;
        return *this;
    }

    inline ColorRGBAf& operator-=(const ColorRGBAf& _p)
    {
        r -= _p.r;
        g -= _p.g;
        b -= _p.b;
        return *this;
    }

    inline ColorRGBAf& operator/=(const int d)
    {
        r /= d;
        g /= d;
        b /= d;
        return *this;
    }
};

} // namespace aliceVision
