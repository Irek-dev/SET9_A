#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr std::size_t kAlphabet = 256;
constexpr std::size_t kBuckets = kAlphabet + 1;
constexpr std::size_t kSwitchThreshold = 74;

int charAt(const std::string& value, std::size_t depth) {
    if (depth >= value.size()) {
        return -1;
    }
    return static_cast<unsigned char>(value[depth]);
}

void stringQuickSort(std::vector<std::string>& values, int left, int right, std::size_t depth) {
    if (left >= right) {
        return;
    }

    int less = left;
    int greater = right;
    int i = left;
    const int pivot = charAt(values[static_cast<std::size_t>(left + (right - left) / 2)], depth);

    while (i <= greater) {
        const int current = charAt(values[static_cast<std::size_t>(i)], depth);
        if (current < pivot) {
            std::swap(values[static_cast<std::size_t>(less)], values[static_cast<std::size_t>(i)]);
            ++less;
            ++i;
        } else if (current > pivot) {
            std::swap(values[static_cast<std::size_t>(i)], values[static_cast<std::size_t>(greater)]);
            --greater;
        } else {
            ++i;
        }
    }

    stringQuickSort(values, left, less - 1, depth);
    if (pivot >= 0) {
        stringQuickSort(values, less, greater, depth + 1);
    }
    stringQuickSort(values, greater + 1, right, depth);
}

void msdRadixQuickSort(std::vector<std::string>& values,
                       std::vector<std::string>& buffer,
                       std::size_t left,
                       std::size_t right,
                       std::size_t depth) {
    if (right - left < 2) {
        return;
    }
    if (right - left < kSwitchThreshold) {
        stringQuickSort(values, static_cast<int>(left), static_cast<int>(right - 1), depth);
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
            msdRadixQuickSort(values, buffer, bucketLeft, bucketRight, depth + 1);
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
    msdRadixQuickSort(values, buffer, 0, values.size(), 0);

    for (const std::string& value : values) {
        std::cout << value << '\n';
    }
    return 0;
}
