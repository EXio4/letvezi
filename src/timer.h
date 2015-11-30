#pragma once

#include <set>
#include <functional>

class Timer {
private:
    struct TimInf {
        int rem_ms = 0;
        std::function<void()> cb;
    };
    std::list<TimInf> s;
public:
    Timer() {};
    void add_timer(int ms, std::function<void()> fn) {
        s.push_back(TimInf{ms, fn});
    };
    void advance(int ms_fact) {
        for (auto i = s.begin(); i != s.end() ; ) {
            i->rem_ms -= ms_fact;
            if (i->rem_ms <= 0) {
                i->cb();
                i = s.erase(i);
            } else {
                i++;
            };
        };
    };
};