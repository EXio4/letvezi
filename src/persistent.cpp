
#include "persistent.h"

/*
   file format:
        [MAGIC_NUMBER     ] 32 bits
        [NUMBER_OF_CHUNKS ] 8 bits
        [CHUNK]           variable
    every chunk is defined as:
        [CHUNK_TYPE]         8 bits
        [CHUNK_DATA_LEN]    32 bits
        [CHUNK_DATA]        data from the chunk
    there are (currently) two kinds of chunks
        chunk_type = 0x01 (Player Configuration)
        chunk_type = 0x02 (High scores)
*/

uint32_t persistent_magic_number = 0x13423701;


Persistent::Chunk Persistent::HighScores::serialize_chunk() {
    Binary::Data w = Binary::serialize<uint32_t>(arr.size());
    for (auto& e : arr) {
        w += Binary::serialize<std::string>(e.name  );
        w += Binary::serialize<uint64_t>   (e.points);
    };
    return Persistent::Chunk(CHighScores, w);
};
Persistent::Chunk Persistent::UserData::serialize_chunk() {
    Binary::Data w = Binary::serialize<std::string>(player_name);
    return Persistent::Chunk(CUserData, w);
};

bool Persistent::HighScores::load_chunk(Chunk& chunk) {
    if (chunk.type != CHighScores) return false;
    uint32_t size = Binary::deserialize<uint32_t>(chunk.dat);
    arr.clear();
    for (uint32_t i=0; i<size;i++) {
        Entry e;
        e.name   = Binary::deserialize<std::string>(chunk.dat);
        e.points = Binary::deserialize<uint64_t>   (chunk.dat);
        arr.push_back(e);
    };
    return true;
};
bool Persistent::UserData::load_chunk(Chunk& chunk) {
    if (chunk.type != CUserData) return false;
    player_name = Binary::deserialize<std::string>(chunk.dat);
    return true;
};


void Persistent::HighScores::add_score(std::string player_name, uint64_t points) {
    arr.push_back(Entry{player_name, points});
    std::sort(arr.begin(), arr.end(), [](Entry x, Entry y) { return x.points > y.points; });
    if (arr.size() > 10) {
        arr.resize(10);
    };
};

void Persistent::serialize_to(std::string file_name) {
    std::fstream fh(file_name , std::ios::out | std::ios::binary);
    Binary::Data w = Binary::serialize<uint32_t>(persistent_magic_number) +
                     Binary::serialize<uint8_t> (2); // high_scores + user_data

    w += high_scores.serialize_chunk().serialize();
    w += user_data.serialize_chunk().serialize();
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
    auto number_of_chunks = Binary::deserialize<uint8_t>(w);
    for (auto i=0; i<number_of_chunks; i++) {
        Chunk chunk(w);
        { HighScores hs;
          if (hs.load_chunk(chunk)) {
              std::cout << "Loading high scores" << std::endl;
              high_scores = hs;
              continue;
          };
        }
        { UserData ud;
          if (ud.load_chunk(chunk)) {
              std::cout << "Loading user data" << std::endl;
              user_data = ud;
              continue;
          };
        };
        std::cout << "Found unknown chunk format on file" << std::endl;
    };
    return true;
};
