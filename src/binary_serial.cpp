
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

    template<> Data serialize(std::string str) {
        Data w = serialize<uint32_t>(str.size());
        for (auto& c : str) {
            w += serialize<uint8_t>(c);
        };
        return w;
    };
    template<> std::string deserialize(Data& d) {
        std::string str = "";
        {
            auto size = deserialize<uint32_t>(d);
            for (uint32_t j=0; j<size; j++) {
                str.push_back(Binary::deserialize<uint8_t>(d));
            };
        };
        return str;
    };
};