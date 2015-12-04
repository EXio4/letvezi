#pragma once

#include <functional>
#include <iostream>
#include "binary_serial.h"

struct Persistent {
    private:
        std::string file_name;
        struct HighScores {
                struct Entry {
                    std::string    name = "xxx";
                    uint64_t       points = 0;
                };
                std::vector<Entry> arr;
                void add_score(std::string, uint64_t);
                const std::vector<Entry> table() { return arr; };
                Binary::Data serialize();
                void load(Binary::Data&);
                bool valid();
        };
    public:
        HighScores high_scores;
        enum ChunkType : uint8_t {
            HighScores = 0x01,
            UserData   = 0x02
        };
        Persistent(std::string file) : file_name(file) {};
        void serialize_to(std::string);
        bool load_from(std::string);
        void save() { return serialize_to(file_name); };
        bool load() { return load_from(file_name);    };
};


/*
   file format:
        [MAGIC_NUMBER     ] 32 bits
        [NUMBER_OF_CHUNKS ] 8 bits
        [CHUNK]           variable
    every chunk is defined as:
        [CHUNK_TYPE]         8 bits
        [CHUNK_DATA_LEN]    16 bits
        [CHUNK_DATA]        data from the chunk
    there are (currently) two kinds of chunks
        chunk_type = 0x01 (Player Configuration)
        chunk_type = 0x11 (High scores)
*/

uint32_t persistent_magic_number = 0x13423701;

void Persistent::HighScores::add_score(std::string player_name, uint64_t points) {
    arr.push_back(Entry{player_name, points});
    std::sort(arr.begin(), arr.end(), [](Entry x, Entry y) { return x.points > y.points; });
    if (arr.size() > 10) {
        arr.resize(10);
    };
};

bool Persistent::HighScores::valid() {
    if (arr.size() >= 255) return false;
    for (auto& s : arr) {
        if (s.name.size() >= 255) return false;
    };
    return true;
};

void Persistent::serialize_to(std::string file_name) {
    std::fstream fh(file_name , std::ios::out | std::ios::binary);
    if (!high_scores.valid()) return;
    Binary::Data w = Binary::serialize<uint32_t>(persistent_magic_number) +
                     Binary::serialize<uint8_t> (high_scores.arr.size());
    for (auto& s : high_scores.arr) {
        w += Binary::serialize<std::string>(s.name  );
        w += Binary::serialize<uint64_t>   (s.points);
    };
    w.writeTo(fh);
};

bool Persistent::load_from(std::string file_name) {
    std::fstream fh(file_name , std::ios::in | std::ios::binary);
    if (!fh.is_open()) return false;
    Binary::Data w;
    w.loadFrom(fh);
    {
      if (Binary::deserialize<uint32_t>(w) != persistent_magic_number) return false;
    };
    high_scores.arr.clear();
    auto number_of_chunks = Binary::deserialize<uint8_t>(w);
    for (auto i=0; i<number_of_chunks; i++) {
        auto str      = Binary::deserialize<std::string>(w);
        auto f_points = Binary::deserialize<uint64_t>   (w);
        high_scores.arr.push_back(HighScores::Entry{str,f_points});
    };
    return true;
};
