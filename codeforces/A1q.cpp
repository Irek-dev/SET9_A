#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

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

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int n = 0;
    std::cin >> n;
    std::vector<std::string> values(static_cast<std::size_t>(n));
    for (std::string& value : values) {
        std::cin >> value;
    }

    if (!values.empty()) {
        stringQuickSort(values, 0, static_cast<int>(values.size() - 1), 0);
    }

    for (const std::string& value : values) {
        std::cout << value << '\n';
    }
    return 0;
}
