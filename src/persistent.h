#pragma once

#include <functional>
#include <iostream>
#include <fstream>
#include "binary_serial.h"

struct Persistent {
    private:
        std::string file_name;
        enum ChunkType : uint8_t {
            CHighScores = 0x01,
            CUserData   = 0x02
        };
        struct Chunk {
            ChunkType type;
            Binary::Data dat;
            Chunk(ChunkType type, Binary::Data dat) : type(type), dat(dat) {};
            Chunk(Binary::Data& d) { load(d); };
            Binary::Data serialize() {
                Binary::Data w = Binary::serialize<uint8_t>(type);
                w += Binary::serialize<uint32_t>(dat.size());
                w += dat;
                return w;
            };
            void load(Binary::Data& d) {
                ChunkType x  = ChunkType(Binary::deserialize<uint8_t> (d));
                uint32_t len = Binary::deserialize<uint32_t>(d);
                type = x;
                dat = d.splitAt(len);
            };
        };
        struct Pers {
            virtual Chunk serialize_chunk()    = 0;
            virtual bool  load_chunk(Chunk& c) = 0;
        };
        struct HighScores : Pers {
                struct Entry {
                    std::string    name = "xxx";
                    uint64_t       points = 0;
                };
                std::vector<Entry> arr;
                void add_score(std::string, uint64_t);
                const std::vector<Entry> table() { return arr; };
                Chunk serialize_chunk();
                bool load_chunk(Chunk& c);
        };
        struct UserData : Pers {
                std::string player_name = "YOU";
                Chunk serialize_chunk();
                bool load_chunk(Chunk& c);
        };
    public:
        HighScores high_scores;
        UserData   user_data;
        Persistent(std::string file) : file_name(file) {
        };
        void serialize_to(std::string);
        bool load_from(std::string);
        void save() { return serialize_to(file_name); };
        bool load() { return load_from(file_name);    };
};
