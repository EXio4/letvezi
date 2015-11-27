#pragma once

template <typename Sca>
struct Vec {
    Sca x;
    Sca y;
    Vec(Sca x, Sca y) : x(x), y(y) {};
};

template <typename Sca>
Vec<Sca> operator+(Vec<Sca> a, Vec<Sca> b) {
    return Vec<Sca>(a.x + b.x, a.y + b.y);
};

template <typename Sca>
Vec<Sca> operator-(Vec<Sca> a, Vec<Sca> b) {
    return Vec<Sca>(a.x - b.x, a.y - b.y);
};

template <typename Sca>
Vec<Sca> operator*(Sca a, Vec<Sca> b) {
    return Vec<Sca>(a * b.x, a * b.y);
};
