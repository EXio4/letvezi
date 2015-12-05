
#include "binary_serial.h"
#include <queue>

namespace Binary {
    Data operator+(Data x, Data y) {
        if (y.q.empty()) return x;
        Data w;
        w.q = x.q;
        for (auto e = y.q.front() ; !y.q.empty() ; y.q.pop()) {
            w.q.push(e);
        };
        return w;
    }
    Data& operator+=(Data& x, Data y) {
        while (!y.q.empty()) {
            x.q.push(y.getByte());
        };
        return x;
    };
};