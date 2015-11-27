#pragma once

template <typename T>
struct Vec {
    typename T::scalar x;
    typename T::scalar y;
    Vec(typename T::scalar x, typename T::scalar y) : x(x), y(y) {};
};

template <typename T>
Vec<T> operator+(Vec<T> a, Vec<T> b) {
    return Vec<T>(a.x + b.x, a.y + b.y);
};

template <typename T>
Vec<T> operator-(Vec<T> a, Vec<T> b) {
    return Vec<T>(a.x - b.x, a.y - b.y);
};

template <typename T>
Vec<T> operator*(typename T::scalar a, Vec<T> b) {
    return Vec<T>(a * b.x, a * b.y);
};
