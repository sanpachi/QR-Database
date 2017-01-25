#include <sys/stat.h>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define SFMT_MEXP 19937
#include "./SFMT/SFMT.h"
#include "./parallel-radix-sort/parallel_radix_sort.h"

using std::cin;
using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::stringstream;
using std::setfill;
using std::setw;

const uint64_t MAX = 0x100000000;
const uint32_t BLOCK_SIZE = 0x01000000;
const uint32_t SPLIT_SIZE = 100;

template <typename T>
string toHex(T num, uint32_t radix) {
    std::stringstream ss;
    ss << std::hex << std::setw(radix) << std::setfill('0') << num;
    return ss.str();
}

uint32_t encode(uint64_t rand[]) {
    uint64_t r = 0;
    for (int i = 0; i < 7; i++) {
        r = r * 17 + (rand[i] % 17);
    }
    return r;
}
void skipSFMT(sfmt_t* sfmt) {
    for (int i = 0; i < 417; i++) sfmt_genrand_uint64(sfmt);
}

uint32_t result(uint32_t seed) {
    sfmt_t sfmt;
    sfmt_init_gen_rand(&sfmt, seed);
    skipSFMT(&sfmt);
    uint64_t rand[7];
    for (int i = 0; i < 7; i++) {
        rand[i] = sfmt_genrand_uint64(&sfmt) % 17;
    }
    return encode(rand);
}

string filename(int i, const string& suffix = "") {
    stringstream ss;
    ss << "bin/QR" << suffix;
    ss << setfill('0') << setw(2) << i;
    ss << ".bin";
    return ss.str();
}

off_t fileSize(const string& fname) {
    struct stat st;
    stat(fname.c_str(), &st);
    return st.st_size;
}

int create() {
    FILE* fps[SPLIT_SIZE];
    for (int i = 0; i < SPLIT_SIZE; i++) {
        auto fname = filename(i);
        fps[i] = fopen(fname.c_str(), "wb");
    }
    vector<uint64_t> entries;
    entries.resize(BLOCK_SIZE);
    for (uint64_t count = 0; count < MAX; count += BLOCK_SIZE) {
        if (count % 0x100000 == 0) cout << toHex(count, 8) << endl;
#pragma omp parallel for
        for (int i = 0; i < BLOCK_SIZE; i++) {
            auto seed = count + i;
            entries[i] = (seed << 32) | result(seed);
        }
        for (int i = 0; i < BLOCK_SIZE; i++) {
            uint32_t last = (entries[i] & 0xFFFFFFFF) % SPLIT_SIZE;
            fwrite(&entries[i], sizeof(uint64_t), 1, fps[last]);
        }
    }

    return 0;
}

template <typename T>
vector<T> fetchEntries(int i) {
    auto fname = filename(i);
    FILE* fp = fopen(fname.c_str(), "rb");
    off_t sz = fileSize(fname.c_str()) / sizeof(T);

    vector<T> entries(sz);
    fread(entries.data(), sizeof(T), sz, fp);
    fclose(fp);

    return entries;
}

template <typename T>
void writeEntries(int i, const vector<T>& entries) {
    auto fname = filename(i);
    FILE* fp = fopen(fname.c_str(), "wb");

    fwrite(entries.data(), sizeof(T), entries.size(), fp);

    fclose(fp);
}

int sort() {
    for (uint32_t i = 0; i < SPLIT_SIZE; i++) {
        printf("i = %d\n", i);
        auto entries = fetchEntries<uint64_t>(i);
        auto sz = entries.size();

        vector<uint32_t> results(sz);
        vector<uint32_t> seeds(sz);
        for (int j = 0; j < sz; j++) {
            seeds[j] = entries[j] >> 32;
            results[j] = entries[j] & 0xFFFFFFFF;
        }
        parallel_radix_sort::PairSort<uint32_t, uint32_t> pair_sort;
        pair_sort.Init(sz);
        auto sorted = pair_sort.Sort(results.data(), seeds.data(), sz);
        for (int j = 0; j < sz; j++) {
            seeds[j] = sorted.second[j];
        }
        writeEntries<uint32_t>(i, seeds);
    }
    return 0;
}
template <typename T>
T fetch(uint32_t pos, FILE* fp) {
    T ret;
    fseek(fp, 1LL * pos * sizeof(T), SEEK_SET);
    fread(&ret, sizeof(T), 1, fp);
    return ret;
}
bool ok(uint32_t seed, uint64_t* rand, int len) {
    sfmt_t sfmt;
    sfmt_init_gen_rand(&sfmt, seed);
    skipSFMT(&sfmt);
    for (int i = 0; i < len; i++) {
        uint64_t r = sfmt_genrand_uint64(&sfmt);
        if (r % 17 != rand[i]) return false;
    }
    return true;
}

int search() {
    uint64_t rand[15];
    int n;
    cin >> n;
    for (int i = 0; i < n; i++) {
        cin >> rand[i];
    }
    auto r = encode(rand);
    auto fname = filename(static_cast<int>(r % SPLIT_SIZE));
    FILE* fp = fopen(fname.c_str(), "rb");
    auto sz = fileSize(fname) / sizeof(uint32_t);

    uint32_t pos = 0;
    for (int i = 31; i >= 0; i--) {
        auto nextPos = pos + (1 << i);
        if (nextPos < sz) {
            auto seed = fetch<uint32_t>(nextPos, fp);
            if (result(seed) <= r) {
                pos = nextPos;
            }
        }
    }

    while (1) {
        auto seed = fetch<uint32_t>(pos, fp);
        auto res = result(seed);
        if (res == r) {
            if (ok(seed, rand, n)) {
                cout << toHex(seed, 8) << " " << toHex(res, 8) << endl;
            }
        } else {
            break;
        }
        if (pos == 0) break;
        pos--;
    }
    fclose(fp);

    return 0;
}

int test() {
    auto fname = filename(1);
    FILE* fp = fopen(fname.c_str(), "rb");
    for (int i = 1; i < 20; i++) {
        auto r1 = result(fetch<uint32_t>(i - 1, fp));
        auto r2 = result(fetch<uint32_t>(i, fp));
        if (r1 > r2) return 1;
    }

    return 0;
}

int main(int argc, char** argv) {
    if (argc > 1 && strcmp(argv[1], "create") == 0) {
        return create();
    } else if (argc > 1 && strcmp(argv[1], "sort") == 0) {
        return sort();
    } else if (argc > 1 && strcmp(argv[1], "test") == 0) {
        return test();
    } else {
        return search();
    }
}
