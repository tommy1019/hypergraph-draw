#pragma once

struct Vec2f {
    double x;
    double y;

    double length() { return std::sqrt(x * x + y * y); }

    Vec2f normalized() { return Vec2f{ x, y } / length(); }

    Vec2f rot90() { return Vec2f{ -y, x }; }

    Vec2f operator+=(const Vec2f& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    Vec2f operator-=(const Vec2f& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    friend Vec2f operator+(const Vec2f& lhs, const Vec2f& rhs) {
        Vec2f res;
        res.x = lhs.x + rhs.x;
        res.y = lhs.y + rhs.y;
        return res;
    }

    friend Vec2f operator-(const Vec2f& lhs, const Vec2f& rhs) {
        Vec2f res;
        res.x = lhs.x - rhs.x;
        res.y = lhs.y - rhs.y;
        return res;
    }

    template <typename T>
    friend Vec2f operator*(const Vec2f& lhs, T rhs) {
        Vec2f res;
        res.x = lhs.x * rhs;
        res.y = lhs.y * rhs;
        return res;
    }
    template <typename T>
    friend Vec2f operator*(T lhs, const Vec2f& rhs) {
        Vec2f res;
        res.x = lhs * rhs.x;
        res.y = lhs * rhs.y;
        return res;
    }

    template <typename T>
    friend Vec2f operator/(const Vec2f& lhs, T rhs) {
        Vec2f res;
        res.x = lhs.x / rhs;
        res.y = lhs.y / rhs;
        return res;
    }
};