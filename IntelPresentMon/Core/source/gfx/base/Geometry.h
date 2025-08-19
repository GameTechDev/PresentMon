// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <string>
#include <format>
#include <array>

namespace p2c::gfx
{
    template<typename T>
    struct Vec2T
    {
        Vec2T operator+(const Vec2T& rhs) const
        {
            return { x + rhs.x, y + rhs.y };
        }
        Vec2T operator-(const Vec2T& rhs) const
        {
            return { x - rhs.x, y - rhs.y };
        }
        template<typename S>
        operator Vec2T<S>() const
        {
            return { S(x), S(y) };
        }
        T x, y;
    };

    using Vec2 = Vec2T<float>;
    using Vec2I = Vec2T<int>;

    template<typename T>
    struct DimensionsT
    {
        DimensionsT() = default;
        DimensionsT(T width, T height) : width{ width }, height{ height } {}
        bool operator==(const DimensionsT& rhs) const noexcept
        {
            return width == rhs.width && height == rhs.height;
        }
        bool operator<(const DimensionsT & rhs) const noexcept
        {
            return GetArea() < rhs.GetArea();
        }
        operator bool() const
        {
            return width != T(0) || height != T(0);
        }
        DimensionsT operator-() const { return { -width, -height }; }
        DimensionsT operator+(const DimensionsT& rhs) const { return { width + rhs.width, height + rhs.height }; }
        DimensionsT operator-(const DimensionsT& rhs) const { return *this + (-rhs); }
        DimensionsT operator*(const T& factor) const
        {
            auto copy = *this;
            copy.width *= factor;
            copy.height *= factor;
            return copy;
        }
        DimensionsT operator/(const T& divisor) const
        {
            auto copy = *this;
            copy.width /= divisor;
            copy.height /= divisor;
            return copy;
        }
        template<typename S>
        operator DimensionsT<S>() const
        {
            return { S(width), S(height) };
        }
        T GetArea() const
        {
            return width * height;
        }
        std::wstring ToString() const { return std::format(L"w:{} h:{}", width, height); }
        T width, height;
    };

    using Dimensions = DimensionsT<float>;
    using DimensionsI = DimensionsT<int>;

    template<typename T>
    struct SkirtT
    {
        SkirtT() = default;
        SkirtT(T size) : SkirtT{ size, size, size, size } {}
        SkirtT(T left, T top, T right, T bottom) : left{ left }, top{ top }, right{ right }, bottom{ bottom } {}
        SkirtT(T horizontal, T vertical) : left{ horizontal }, top{ vertical }, right{ horizontal }, bottom{ vertical } {}
        DimensionsT<T> ToDimensions() const { return { left + right, top + bottom }; }
        SkirtT operator-() const
        {
            return {-left, -top, -right, -bottom};
        }
        SkirtT operator+(const SkirtT& rhs) const
        {
            return { left + rhs.left, top + rhs.top, right + rhs.right, bottom + rhs.bottom };
        }
        SkirtT operator-(const SkirtT& rhs) const
        {
            return *this + (-rhs);
        }
        operator bool() const
        {
            return left != T(0) || top != T(0) || right != T(0) || bottom != T(0);
        }
        bool IsUniform() const
        {
            return left == right && left == top && left == bottom;
        }

        T left = {}, top = {}, right = {}, bottom = {};
    };

    using Skirt = SkirtT<float>;
    using SkirtI = SkirtT<int>;

    template<typename T>
    struct RectT
    {
        RectT() = default;
        RectT(T left, T top, T right, T bottom) : left{ left }, top{ top }, right{ right }, bottom{ bottom } {}
        RectT(const Vec2T<T>& topLeft, const DimensionsT<T>& dims)
            :
            left{ topLeft.x },
            top{ topLeft.y },
            right{ left + dims.width },
            bottom{ top + dims.height }
        {}
        template<typename S>
        operator RectT<S>() const
        {
            return { S(left), S(top), S(right), S(bottom) };
        }
        Vec2T<T> GetTopLeft() const { return { left, top }; }
        Vec2T<T> GetBottomRight() const { return { right, bottom }; }
        Vec2T<T> GetTopRight() const { return { right, top }; }
        Vec2T<T> GetBottomLeft() const { return { left, bottom }; }
        DimensionsT<T> GetDimensions() const { return { right - left, bottom - top }; }
        RectT Augment(const SkirtT<T>& skirt) const
        { 
            return {
                left - skirt.left,
                top - skirt.top,
                right + skirt.right,
                bottom + skirt.bottom
            };
        }
        bool Contains(const RectT& inside) const
        {
            return inside.left >= left && inside.top >= top && inside.right <= right && inside.bottom <= bottom;
        }
        bool operator==(const RectT& rhs) const
        {
            return left == rhs.left && right == rhs.right && top == rhs.top && bottom == rhs.bottom;
        }
        std::wstring ToString() const { return std::format(L"l:{} t:{} r:{} b:{}", left, top, right, bottom); }
        T left, top, right, bottom;
    };

    using Rect = RectT<float>;
    using RectI = RectT<int>;

    struct Color
    {
        static Color FromBytes(uint8_t r, uint8_t g, uint8_t b)
        {
            return FromBytes(r, g, b, 255);
        }
        static Color FromBytes(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        {
            return { float(r) / 255.f, float(g) / 255.f, float(b) / 255.f, float(a) / 255.f };
        }
        Color operator*(float rhs) const
        {
            return { r * rhs, g * rhs, b * rhs, a };
        }
        Color WithAlpha(float a) const
        {
            return { r,g,b,a };
        }
        bool IsVisible() const
        {
            return a != 0.f;
        }
        bool operator==(const Color& rhs) const
        {
            return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
        }
        float r = 0.f, g = 0.f, b = 0.f, a = 0.f;

        static constexpr Color White(float alpha = 1.f)
        {
            return { 1.f, 1.f, 1.f, alpha };
        }
        static constexpr Color Black(float alpha = 1.f)
        {
            return { 0.f, 0.f, 0.f, alpha };
        }
        static constexpr Color Red(float alpha = 1.f)
        {
            return { 1.f, 0.f, 0.f, alpha };
        }
        static constexpr Color Green(float alpha = 1.f)
        {
            return { 0.f, 1.f, 0.f, alpha };
        }
        static constexpr Color Blue(float alpha = 1.f)
        {
            return { 0.f, 0.f, 1.f, alpha };
        }
        std::wstring ToWString() const
        {
            return std::format(L"r{:.2f}g{:.2f}b{:.2f}a{:.2f}", r, g, b, a);
        }
        std::array<float, 4> AsArray() const
        {
            return { r, g, b, a };
        }
    };

    template<typename T>
    T CalculateCenteredLeadingEdge(T start, T end, T size)
    {
        return (start + end - size) / 2.0f;
    }
}
