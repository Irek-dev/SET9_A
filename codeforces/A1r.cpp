#include <array>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr std::size_t kAlphabet = 256;
constexpr std::size_t kBuckets = kAlphabet + 1;

int charAt(const std::string& value, std::size_t depth) {
    if (depth >= value.size()) {
        return -1;
    }
    return static_cast<unsigned char>(value[depth]);
}

void msdRadixSort(std::vector<std::string>& values,
                  std::vector<std::string>& buffer,
                  std::size_t left,
                  std::size_t right,
                  std::size_t depth) {
    if (right - left < 2) {
        return;
    }

    std::array<std::size_t, kBuckets> count{};
    for (std::size_t i = left; i < right; ++i) {
        const int c = charAt(values[i], depth);
        const std::size_t bucket = static_cast<std::size_t>(c + 1);
        ++count[bucket];
    }

    std::array<std::size_t, kBuckets + 1> prefix{};
    for (std::size_t bucket = 0; bucket < kBuckets; ++bucket) {
        prefix[bucket + 1] = prefix[bucket] + count[bucket];
    }

    std::array<std::size_t, kBuckets> next{};
    for (std::size_t bucket = 0; bucket < kBuckets; ++bucket) {
        next[bucket] = prefix[bucket];
    }

    for (std::size_t i = left; i < right; ++i) {
        const int c = charAt(values[i], depth);
        const std::size_t bucket = static_cast<std::size_t>(c + 1);
        buffer[left + next[bucket]++] = values[i];
    }
    for (std::size_t i = left; i < right; ++i) {
        values[i] = buffer[i];
    }

    for (std::size_t bucket = 1; bucket < kBuckets; ++bucket) {
        const std::size_t bucketLeft = left + prefix[bucket];
        const std::size_t bucketRight = left + prefix[bucket + 1];
        if (bucketRight - bucketLeft > 1) {
            msdRadixSort(values, buffer, bucketLeft, bucketRight, depth + 1);
        }
    }
}

}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int n = 0;
    std::cin >> n;
    std::vector<std::string> values(static_cast<std::size_t>(n));
    for (std::string& value : values) {
        std::cin >> value;
    }

    std::vector<std::string> buffer(values.size());
    msdRadixSort(values, buffer, 0, values.size(), 0);

    for (const std::string& value : values) {
        std::cout << value << '\n';
    }
    return 0;
}
