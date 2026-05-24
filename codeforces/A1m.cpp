#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

struct CompareResult {
    int cmp = 0;
    std::size_t lcp = 0;
};

CompareResult compareFrom(const std::string& lhs, const std::string& rhs, std::size_t start) {
    std::size_t pos = start;
    while (pos < lhs.size() && pos < rhs.size()) {
        const unsigned char leftChar = static_cast<unsigned char>(lhs[pos]);
        const unsigned char rightChar = static_cast<unsigned char>(rhs[pos]);
        if (leftChar < rightChar) {
            return {-1, pos};
        }
        if (leftChar > rightChar) {
            return {1, pos};
        }
        ++pos;
    }
    if (lhs.size() == rhs.size()) {
        return {0, pos};
    }
    return lhs.size() < rhs.size() ? CompareResult{-1, pos} : CompareResult{1, pos};
}

struct LcpRun {
    std::vector<std::string> values;
    std::vector<std::size_t> lcpPrev;
};

bool equalsLast(const std::string& last, const std::string& candidate, std::size_t lcp) {
    return lcp == last.size() && lcp == candidate.size();
}

void appendValue(LcpRun& output, const std::string& value, std::size_t lcpWithPrevious) {
    output.values.push_back(value);
    output.lcpPrev.push_back(output.values.size() == 1 ? 0 : lcpWithPrevious);
}

LcpRun mergeRuns(const LcpRun& left, const LcpRun& right) {
    LcpRun output;
    output.values.reserve(left.values.size() + right.values.size());
    output.lcpPrev.reserve(left.values.size() + right.values.size());

    std::size_t i = 0;
    std::size_t j = 0;
    std::size_t leftLcp = 0;
    std::size_t rightLcp = 0;

    while (i < left.values.size() && j < right.values.size()) {
        bool takeLeft = true;
        std::size_t lcpBetween = 0;
        std::size_t winnerLcp = 0;

        if (output.values.empty()) {
            const CompareResult result = compareFrom(left.values[i], right.values[j], 0);
            takeLeft = result.cmp <= 0;
            lcpBetween = result.lcp;
        } else {
            const std::string& last = output.values.back();
            const bool leftEqualLast = equalsLast(last, left.values[i], leftLcp);
            const bool rightEqualLast = equalsLast(last, right.values[j], rightLcp);

            if (leftEqualLast && rightEqualLast) {
                takeLeft = true;
                lcpBetween = last.size();
            } else if (leftEqualLast) {
                takeLeft = true;
                lcpBetween = rightLcp;
            } else if (rightEqualLast) {
                takeLeft = false;
                lcpBetween = leftLcp;
            } else if (leftLcp > rightLcp) {
                takeLeft = true;
                lcpBetween = rightLcp;
            } else if (leftLcp < rightLcp) {
                takeLeft = false;
                lcpBetween = leftLcp;
            } else {
                const CompareResult result = compareFrom(left.values[i], right.values[j], leftLcp);
                takeLeft = result.cmp <= 0;
                lcpBetween = result.lcp;
            }
        }

        if (takeLeft) {
            winnerLcp = output.values.empty() ? 0 : leftLcp;
            appendValue(output, left.values[i], winnerLcp);
            ++i;
            if (i < left.values.size()) {
                leftLcp = left.lcpPrev[i];
            }
            rightLcp = lcpBetween;
        } else {
            winnerLcp = output.values.empty() ? 0 : rightLcp;
            appendValue(output, right.values[j], winnerLcp);
            ++j;
            if (j < right.values.size()) {
                rightLcp = right.lcpPrev[j];
            }
            leftLcp = lcpBetween;
        }
    }

    while (i < left.values.size()) {
        appendValue(output, left.values[i], output.values.empty() ? 0 : leftLcp);
        ++i;
        if (i < left.values.size()) {
            leftLcp = left.lcpPrev[i];
        }
    }
    while (j < right.values.size()) {
        appendValue(output, right.values[j], output.values.empty() ? 0 : rightLcp);
        ++j;
        if (j < right.values.size()) {
            rightLcp = right.lcpPrev[j];
        }
    }

    return output;
}

LcpRun sortRecursive(const std::vector<std::string>& values, std::size_t left, std::size_t right) {
    if (right - left == 1) {
        return LcpRun{{values[left]}, {0}};
    }
    const std::size_t middle = left + (right - left) / 2;
    LcpRun leftRun = sortRecursive(values, left, middle);
    LcpRun rightRun = sortRecursive(values, middle, right);
    return mergeRuns(leftRun, rightRun);
}

void stringMergeSort(std::vector<std::string>& values) {
    if (values.size() < 2) {
        return;
    }
    values = sortRecursive(values, 0, values.size()).values;
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

    stringMergeSort(values);

    for (const std::string& value : values) {
        std::cout << value << '\n';
    }
    return 0;
}
