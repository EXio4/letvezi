#pragma once

#include <functional>
#include <iostream>
#include <boost/endian/arithmetic.hpp>

struct HighScores {
private:
    struct Entry {
        std::string                     name = "xxx";
        boost::endian::little_uint64_t  points = 0;
    };
    std::vector<Entry> arr;
    std::string file_name;
    bool valid();
public:
    HighScores(std::string file) : file_name(file) {};
    void serialize_to(std::string);
    bool load_from(std::string);
    void save() { return serialize_to(file_name); };
    bool load() { return load_from(file_name);    };
    void add_score(std::string, uint64_t);
    const std::vector<Entry> table() { return arr; };
};


/*
   file format:
        [MAGIC_NUMBER     ] 32 bits
        [NUMBER_OF_ENTRIES] 8 bits
        [ENTRIES]           variable
   an Entry is represented as:
        [{x:NAME_SIZE}]         8 bits
        [CHAR]                  x bytes
        [SCORE]                 64 bits
*/

boost::endian::little_uint32_t high_scores_magic = 0x13423701;

bool HighScores::valid() {
    if (arr.size() >= 255) return false;
    for (auto& s : arr) {
        if (s.name.size() >= 255) return false;
    };
    return true;
};

void HighScores::serialize_to(std::string file_name) {
    std::fstream fh(file_name , std::ios::out | std::ios::binary);
    if (!valid()) return;
    boost::endian::little_uint8_t size = arr.size();
    fh.write((char*)&high_scores_magic, sizeof(boost::endian::little_uint32_t));
    fh.write((char*)&size             , sizeof(boost::endian::little_uint8_t ));
    for (auto& s : arr) {
        boost::endian::little_uint8_t size = s.name.size();
        fh.write((char*)&size, sizeof(boost::endian::little_uint8_t));
        for (auto& c : s.name) {
            boost::endian::little_uint8_t c_ = c;
            fh.write((char*)&c_,  sizeof(boost::endian::little_uint8_t));
        };
        fh.write((char*)&s.points, sizeof(boost::endian::little_uint64_t));
    };
};

bool HighScores::load_from(std::string file_name) {
    std::fstream fh(file_name , std::ios::in | std::ios::binary);
    if (!fh.is_open()) return false;
    {
      boost::endian::little_uint32_t x;
      fh.read((char*)&x, sizeof(boost::endian::little_uint32_t));
      if (x != high_scores_magic) return false;
    };
    arr.clear();
    boost::endian::little_uint8_t number_of_chunks;
    fh.read((char*)&number_of_chunks, sizeof(boost::endian::little_uint8_t));
    for (boost::endian::little_uint8_t i=0; i<number_of_chunks; i++) {
        std::string str = "";
        {
            boost::endian::little_uint8_t name_size;
            fh.read((char*)&name_size, sizeof(boost::endian::little_uint8_t));
            for (boost::endian::little_uint8_t j=0; j<name_size; j++) {
                boost::endian::little_uint8_t ch;
                fh.read((char*)&ch, sizeof(boost::endian::little_uint8_t));
                str.push_back(ch);
            };
        };
        boost::endian::little_uint64_t f_points;
        fh.read((char*)&f_points, sizeof(boost::endian::little_uint64_t));
        arr.push_back(Entry{str,f_points});
    };
    return true;
};

void HighScores::add_score(std::string player_name, uint64_t points) {
    arr.push_back(Entry{player_name, points});
    std::sort(arr.begin(), arr.end(), [](Entry x, Entry y) { return x.points > y.points; });
    if (arr.size() > 10) {
        arr.resize(10);
    };
};