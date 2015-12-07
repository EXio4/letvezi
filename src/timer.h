#pragma once

#include <map>
#include <functional>

class Timer {
private:
    struct TimInf {
        uint64_t rem_ms = 0;
        std::function<void()> cb;
    };
    struct TimID {
        uint64_t tim_id = 0;
        TimID& operator++() {
            tim_id++;
            return *this;   
        };
        bool operator==(const TimID& other) const {
            return tim_id == other.tim_id;
        };
        bool operator!=(const TimID& other) const {
            return tim_id != other.tim_id;
        };
        bool operator>=(const TimID& other) const {
            return tim_id >= other.tim_id;
        };
        bool operator<=(const TimID& other) const {
            return tim_id <= other.tim_id;
        };
        bool operator< (const TimID& other) const {
            return tim_id <  other.tim_id;
        };
        bool operator> (const TimID& other) const {
            return tim_id >  other.tim_id;
        };
    };
    TimID last_id = TimID{0};
    std::map<TimID, TimInf> s;
public:
    Timer() {};
    TimID add_timer(uint64_t ms, std::function<void()> fn) {
        TimID x = last_id;
        ++last_id;
        s[x] = TimInf{ms, fn};
        return x;
    };
    void remove(TimID xid) {
        s.erase(xid);
    };
    void advance(int ms_fact) {
        for (auto i = s.begin(); i != s.end() ; ) {
            i->second.rem_ms -= ms_fact;
            if (i->second.rem_ms <= 0) {
                i->second.cb();
                i = s.erase(i);
            } else {
                i++;
            };
        };
    };
};