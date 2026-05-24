#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

enum class ArrayType {
    Random,
    ReverseSorted,
    NearlySorted
};

struct SortMetrics {
    std::uint64_t charComparisons = 0;
};

enum class AlgorithmId {
    QuickSort,
    MergeSort,
    StringQuickSort,
    StringMergeSort,
    MsdRadixSort,
    MsdRadixQuickSort
};

class StringGenerator {
public:
    static constexpr std::uint32_t kDefaultSeed = 20260524U;
    static constexpr std::size_t kMinLength = 10U;
    static constexpr std::size_t kMaxLength = 200U;
    static constexpr std::size_t kAlphabetSize = 74U;

    explicit StringGenerator(std::uint32_t seed = kDefaultSeed) : rng_(seed) {}

    std::vector<std::string> generate(std::size_t count) {
        std::uniform_int_distribution<std::size_t> lengthDist(kMinLength, kMaxLength);
        std::uniform_int_distribution<std::size_t> charDist(0, alphabet().size() - 1);

        std::vector<std::string> values;
        values.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            const std::size_t length = lengthDist(rng_);
            std::string current;
            current.reserve(length);
            for (std::size_t j = 0; j < length; ++j) {
                current.push_back(alphabet()[charDist(rng_)]);
            }
            values.push_back(current);
        }
        return values;
    }

    std::vector<std::string> generateDataset(ArrayType type, std::size_t count) {
        std::vector<std::string> values = generate(count);

        if (type == ArrayType::Random) {
            return values;
        }

        sortLexicographically(values);

        if (type == ArrayType::ReverseSorted) {
            std::reverse(values.begin(), values.end());
            return values;
        }

        const std::size_t swaps = std::max<std::size_t>(1, count / 20);
        if (count > 1) {
            std::uniform_int_distribution<std::size_t> indexDist(0, count - 1);
            for (std::size_t i = 0; i < swaps; ++i) {
                const std::size_t first = indexDist(rng_);
                const std::size_t second = indexDist(rng_);
                std::swap(values[first], values[second]);
            }
        }
        return values;
    }

    static const std::string& alphabet() {
        static const std::string chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789"
            "!@#%:;^&*()-";
        return chars;
    }

    static const char* arrayTypeName(ArrayType type) {
        switch (type) {
            case ArrayType::Random:
                return "random";
            case ArrayType::ReverseSorted:
                return "reverse_sorted";
            case ArrayType::NearlySorted:
                return "nearly_sorted";
        }
        return "unknown";
    }

private:
    std::mt19937 rng_;

    void sortLexicographically(std::vector<std::string>& values) const {
        if (values.size() < 2) {
            return;
        }
        std::vector<std::string> buffer(values.size());
        mergeSort(values, buffer, 0, values.size());
    }

    void mergeSort(std::vector<std::string>& values,
                   std::vector<std::string>& buffer,
                   std::size_t left,
                   std::size_t right) const {
        if (right - left < 2) {
            return;
        }
        const std::size_t middle = left + (right - left) / 2;
        mergeSort(values, buffer, left, middle);
        mergeSort(values, buffer, middle, right);

        std::size_t i = left;
        std::size_t j = middle;
        std::size_t k = left;
        while (i < middle && j < right) {
            if (values[i] <= values[j]) {
                buffer[k++] = values[i++];
            } else {
                buffer[k++] = values[j++];
            }
        }
        while (i < middle) {
            buffer[k++] = values[i++];
        }
        while (j < right) {
            buffer[k++] = values[j++];
        }
        for (std::size_t index = left; index < right; ++index) {
            values[index] = buffer[index];
        }
    }
};

namespace {

constexpr std::size_t kRadixAlphabet = 256;
constexpr std::size_t kRadixBuckets = kRadixAlphabet + 1;
constexpr std::size_t kRadixQuickThreshold = 74;

struct CompareResult {
    int cmp = 0;
    std::size_t lcp = 0;
};

int charAt(const std::string& value, std::size_t depth) {
    if (depth >= value.size()) {
        return -1;
    }
    return static_cast<unsigned char>(value[depth]);
}

CompareResult compareStringsFrom(const std::string& lhs,
                                 const std::string& rhs,
                                 std::size_t start,
                                 SortMetrics& metrics) {
    std::size_t pos = start;
    while (pos < lhs.size() && pos < rhs.size()) {
        ++metrics.charComparisons;
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

int compareStrings(const std::string& lhs, const std::string& rhs, SortMetrics& metrics) {
    return compareStringsFrom(lhs, rhs, 0, metrics).cmp;
}

int compareSymbols(int lhs, int rhs, SortMetrics& metrics) {
    if (lhs >= 0 && rhs >= 0) {
        ++metrics.charComparisons;
    }
    if (lhs < rhs) {
        return -1;
    }
    if (lhs > rhs) {
        return 1;
    }
    return 0;
}

void quickSort(std::vector<std::string>& values, int left, int right, SortMetrics& metrics) {
    if (left >= right) {
        return;
    }

    int i = left;
    int j = right;
    const std::string pivot = values[left + (right - left) / 2];

    while (i <= j) {
        while (compareStrings(values[static_cast<std::size_t>(i)], pivot, metrics) < 0) {
            ++i;
        }
        while (compareStrings(values[static_cast<std::size_t>(j)], pivot, metrics) > 0) {
            --j;
        }
        if (i <= j) {
            std::swap(values[static_cast<std::size_t>(i)], values[static_cast<std::size_t>(j)]);
            ++i;
            --j;
        }
    }

    if (left < j) {
        quickSort(values, left, j, metrics);
    }
    if (i < right) {
        quickSort(values, i, right, metrics);
    }
}

void mergeSort(std::vector<std::string>& values,
               std::vector<std::string>& buffer,
               std::size_t left,
               std::size_t right,
               SortMetrics& metrics) {
    if (right - left < 2) {
        return;
    }

    const std::size_t middle = left + (right - left) / 2;
    mergeSort(values, buffer, left, middle, metrics);
    mergeSort(values, buffer, middle, right, metrics);

    std::size_t i = left;
    std::size_t j = middle;
    std::size_t k = left;
    while (i < middle && j < right) {
        if (compareStrings(values[i], values[j], metrics) <= 0) {
            buffer[k++] = values[i++];
        } else {
            buffer[k++] = values[j++];
        }
    }
    while (i < middle) {
        buffer[k++] = values[i++];
    }
    while (j < right) {
        buffer[k++] = values[j++];
    }
    for (std::size_t index = left; index < right; ++index) {
        values[index] = buffer[index];
    }
}

void stringQuickSort(std::vector<std::string>& values,
                     int left,
                     int right,
                     std::size_t depth,
                     SortMetrics& metrics) {
    if (left >= right) {
        return;
    }

    int less = left;
    int greater = right;
    int i = left;
    const int pivot = charAt(values[static_cast<std::size_t>(left + (right - left) / 2)], depth);

    while (i <= greater) {
        const int current = charAt(values[static_cast<std::size_t>(i)], depth);
        const int cmp = compareSymbols(current, pivot, metrics);
        if (cmp < 0) {
            std::swap(values[static_cast<std::size_t>(less)], values[static_cast<std::size_t>(i)]);
            ++less;
            ++i;
        } else if (cmp > 0) {
            std::swap(values[static_cast<std::size_t>(i)], values[static_cast<std::size_t>(greater)]);
            --greater;
        } else {
            ++i;
        }
    }

    stringQuickSort(values, left, less - 1, depth, metrics);
    if (pivot >= 0) {
        stringQuickSort(values, less, greater, depth + 1, metrics);
    }
    stringQuickSort(values, greater + 1, right, depth, metrics);
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

LcpRun mergeLcpRuns(const LcpRun& left, const LcpRun& right, SortMetrics& metrics) {
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
            const CompareResult result = compareStringsFrom(left.values[i], right.values[j], 0, metrics);
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
                const CompareResult result = compareStringsFrom(left.values[i], right.values[j], leftLcp, metrics);
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

LcpRun stringMergeSortRecursive(const std::vector<std::string>& values,
                                std::size_t left,
                                std::size_t right,
                                SortMetrics& metrics) {
    if (right - left == 1) {
        return LcpRun{{values[left]}, {0}};
    }

    const std::size_t middle = left + (right - left) / 2;
    LcpRun leftRun = stringMergeSortRecursive(values, left, middle, metrics);
    LcpRun rightRun = stringMergeSortRecursive(values, middle, right, metrics);
    return mergeLcpRuns(leftRun, rightRun, metrics);
}

void stringMergeSort(std::vector<std::string>& values, SortMetrics& metrics) {
    if (values.size() < 2) {
        return;
    }
    LcpRun sorted = stringMergeSortRecursive(values, 0, values.size(), metrics);
    values = sorted.values;
}

void msdRadixSort(std::vector<std::string>& values,
                  std::vector<std::string>& buffer,
                  std::size_t left,
                  std::size_t right,
                  std::size_t depth,
                  bool switchToQuick,
                  SortMetrics& metrics) {
    if (right - left < 2) {
        return;
    }
    if (switchToQuick && right - left < kRadixQuickThreshold) {
        stringQuickSort(values, static_cast<int>(left), static_cast<int>(right - 1), depth, metrics);
        return;
    }

    std::array<std::size_t, kRadixBuckets> count{};
    for (std::size_t i = left; i < right; ++i) {
        const int c = charAt(values[i], depth);
        if (c >= 0) {
            ++metrics.charComparisons;
        }
        const std::size_t bucket = static_cast<std::size_t>(c + 1);
        ++count[bucket];
    }

    std::array<std::size_t, kRadixBuckets + 1> prefix{};
    for (std::size_t i = 0; i < kRadixBuckets; ++i) {
        prefix[i + 1] = prefix[i] + count[i];
    }

    std::array<std::size_t, kRadixBuckets> next{};
    for (std::size_t i = 0; i < kRadixBuckets; ++i) {
        next[i] = prefix[i];
    }

    for (std::size_t i = left; i < right; ++i) {
        const int c = charAt(values[i], depth);
        const std::size_t bucket = static_cast<std::size_t>(c + 1);
        buffer[left + next[bucket]++] = values[i];
    }
    for (std::size_t i = left; i < right; ++i) {
        values[i] = buffer[i];
    }

    for (std::size_t bucket = 1; bucket < kRadixBuckets; ++bucket) {
        const std::size_t bucketLeft = left + prefix[bucket];
        const std::size_t bucketRight = left + prefix[bucket + 1];
        if (bucketRight - bucketLeft > 1) {
            msdRadixSort(values, buffer, bucketLeft, bucketRight, depth + 1, switchToQuick, metrics);
        }
    }
}

const char* algorithmName(AlgorithmId id) {
    switch (id) {
        case AlgorithmId::QuickSort:
            return "QuickSort";
        case AlgorithmId::MergeSort:
            return "MergeSort";
        case AlgorithmId::StringQuickSort:
            return "StringQuickSort";
        case AlgorithmId::StringMergeSort:
            return "StringMergeSort";
        case AlgorithmId::MsdRadixSort:
            return "MSDRadixSort";
        case AlgorithmId::MsdRadixQuickSort:
            return "MSDRadixQuickSort";
    }
    return "Unknown";
}

std::vector<AlgorithmId> allAlgorithms() {
    return {
        AlgorithmId::QuickSort,
        AlgorithmId::MergeSort,
        AlgorithmId::StringQuickSort,
        AlgorithmId::StringMergeSort,
        AlgorithmId::MsdRadixSort,
        AlgorithmId::MsdRadixQuickSort
    };
}

void sortStrings(std::vector<std::string>& values, AlgorithmId algorithm, SortMetrics& metrics) {
    metrics = SortMetrics{};
    if (values.size() < 2) {
        return;
    }

    switch (algorithm) {
        case AlgorithmId::QuickSort:
            quickSort(values, 0, static_cast<int>(values.size() - 1), metrics);
            return;
        case AlgorithmId::MergeSort: {
            std::vector<std::string> buffer(values.size());
            mergeSort(values, buffer, 0, values.size(), metrics);
            return;
        }
        case AlgorithmId::StringQuickSort:
            stringQuickSort(values, 0, static_cast<int>(values.size() - 1), 0, metrics);
            return;
        case AlgorithmId::StringMergeSort:
            stringMergeSort(values, metrics);
            return;
        case AlgorithmId::MsdRadixSort: {
            std::vector<std::string> buffer(values.size());
            msdRadixSort(values, buffer, 0, values.size(), 0, false, metrics);
            return;
        }
        case AlgorithmId::MsdRadixQuickSort: {
            std::vector<std::string> buffer(values.size());
            msdRadixSort(values, buffer, 0, values.size(), 0, true, metrics);
            return;
        }
    }

    throw std::invalid_argument("unknown algorithm");
}

bool isSortedLexicographically(const std::vector<std::string>& values) {
    for (std::size_t i = 1; i < values.size(); ++i) {
        if (values[i] < values[i - 1]) {
            return false;
        }
    }
    return true;
}

using Clock = std::chrono::steady_clock;

std::vector<ArrayType> allArrayTypes() {
    return {ArrayType::Random, ArrayType::ReverseSorted, ArrayType::NearlySorted};
}

struct Aggregate {
    std::uint64_t timeUs = 0;
    std::uint64_t charComparisons = 0;
    std::size_t runs = 0;
};

std::string csvEscape(const std::string& value) {
    std::string escaped = "\"";
    for (char ch : value) {
        if (ch == '"') {
            escaped += "\"\"";
        } else {
            escaped.push_back(ch);
        }
    }
    escaped.push_back('"');
    return escaped;
}

}

struct ExperimentConfig {
    std::uint32_t seed = StringGenerator::kDefaultSeed;
    std::size_t minSize = 100;
    std::size_t maxSize = 3000;
    std::size_t step = 100;
    std::size_t runs = 10;
};

struct RawResult {
    std::size_t run = 0;
    ArrayType arrayType = ArrayType::Random;
    std::size_t size = 0;
    AlgorithmId algorithm = AlgorithmId::QuickSort;
    std::uint64_t timeUs = 0;
    std::uint64_t charComparisons = 0;
    std::uint32_t seed = 0;
};

class StringSortTester {
public:
    explicit StringSortTester(ExperimentConfig config) : config_(config) {}

    std::vector<RawResult> run() {
        std::vector<RawResult> results;
        results.reserve(config_.runs * allArrayTypes().size() * allAlgorithms().size()
                        * ((config_.maxSize - config_.minSize) / config_.step + 1));

        for (std::size_t runIndex = 1; runIndex <= config_.runs; ++runIndex) {
            const std::uint32_t runSeed =
                static_cast<std::uint32_t>(config_.seed + static_cast<std::uint32_t>(runIndex - 1) * 1009U);

            for (ArrayType type : allArrayTypes()) {
                StringGenerator generator(static_cast<std::uint32_t>(
                    runSeed + static_cast<std::uint32_t>(static_cast<int>(type)) * 131U));
                const std::vector<std::string> fullDataset = generator.generateDataset(type, config_.maxSize);

                for (std::size_t size = config_.minSize; size <= config_.maxSize; size += config_.step) {
                    std::vector<std::string> prefix(fullDataset.begin(), fullDataset.begin() + static_cast<long>(size));

                    for (AlgorithmId algorithm : allAlgorithms()) {
                        std::vector<std::string> working = prefix;
                        SortMetrics metrics;

                        const auto started = Clock::now();
                        sortStrings(working, algorithm, metrics);
                        const auto finished = Clock::now();

                        if (!isSortedLexicographically(working)) {
                            throw std::runtime_error(std::string("algorithm failed: ") + algorithmName(algorithm));
                        }

                        const auto elapsed =
                            std::chrono::duration_cast<std::chrono::microseconds>(finished - started).count();
                        results.push_back(RawResult{
                            runIndex,
                            type,
                            size,
                            algorithm,
                            static_cast<std::uint64_t>(elapsed),
                            metrics.charComparisons,
                            runSeed
                        });
                    }
                }
            }
            std::cerr << "completed run " << runIndex << " / " << config_.runs << '\n';
        }

        return results;
    }

    void writeRawCsv(const std::string& path, const std::vector<RawResult>& results) const {
        std::filesystem::create_directories(std::filesystem::path(path).parent_path());

        std::ofstream out(path);
        if (!out) {
            throw std::runtime_error("cannot write " + path);
        }
        out << "run,array_type,size,algorithm,time_us,char_comparisons,seed\n";
        for (const RawResult& row : results) {
            out << row.run << ','
                << csvEscape(StringGenerator::arrayTypeName(row.arrayType)) << ','
                << row.size << ','
                << csvEscape(algorithmName(row.algorithm)) << ','
                << row.timeUs << ','
                << row.charComparisons << ','
                << row.seed << '\n';
        }
    }

    void writeAveragedCsv(const std::string& path, const std::vector<RawResult>& results) const {
        std::filesystem::create_directories(std::filesystem::path(path).parent_path());

        std::map<std::tuple<std::string, std::size_t, std::string>, Aggregate> aggregates;
        for (const RawResult& row : results) {
            auto key = std::make_tuple(
                std::string(StringGenerator::arrayTypeName(row.arrayType)),
                row.size,
                std::string(algorithmName(row.algorithm)));
            Aggregate& aggregate = aggregates[key];
            aggregate.timeUs += row.timeUs;
            aggregate.charComparisons += row.charComparisons;
            ++aggregate.runs;
        }

        std::ofstream out(path);
        if (!out) {
            throw std::runtime_error("cannot write " + path);
        }
        out << "array_type,size,algorithm,mean_time_us,mean_char_comparisons,runs,seed\n";
        for (const auto& [key, aggregate] : aggregates) {
            const auto& [arrayType, size, algorithm] = key;
            out << csvEscape(arrayType) << ','
                << size << ','
                << csvEscape(algorithm) << ','
                << std::fixed << std::setprecision(3)
                << (static_cast<double>(aggregate.timeUs) / static_cast<double>(aggregate.runs)) << ','
                << (static_cast<double>(aggregate.charComparisons) / static_cast<double>(aggregate.runs)) << ','
                << aggregate.runs << ','
                << config_.seed << '\n';
        }
    }

private:
    ExperimentConfig config_;
};

namespace {

std::size_t parseSize(const char* value) {
    return static_cast<std::size_t>(std::stoull(value));
}

std::uint32_t parseSeed(const char* value) {
    return static_cast<std::uint32_t>(std::stoul(value));
}

void printUsage(const char* program) {
    std::cerr << "usage: " << program
              << " [--runs N] [--seed N] [--min-size N] [--max-size N] [--step N]"
              << " [--raw PATH] [--averaged PATH]\n";
}

}

int main(int argc, char** argv) {
    ExperimentConfig config;
    std::string rawPath = "data/raw_results.csv";
    std::string averagedPath = "data/averaged_results.csv";

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--runs" && i + 1 < argc) {
            config.runs = parseSize(argv[++i]);
        } else if (arg == "--seed" && i + 1 < argc) {
            config.seed = parseSeed(argv[++i]);
        } else if (arg == "--min-size" && i + 1 < argc) {
            config.minSize = parseSize(argv[++i]);
        } else if (arg == "--max-size" && i + 1 < argc) {
            config.maxSize = parseSize(argv[++i]);
        } else if (arg == "--step" && i + 1 < argc) {
            config.step = parseSize(argv[++i]);
        } else if (arg == "--raw" && i + 1 < argc) {
            rawPath = argv[++i];
        } else if (arg == "--averaged" && i + 1 < argc) {
            averagedPath = argv[++i];
        } else if (arg == "--help") {
            printUsage(argv[0]);
            return EXIT_SUCCESS;
        } else {
            printUsage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (config.runs == 0 || config.step == 0 || config.minSize > config.maxSize) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    try {
        StringSortTester tester(config);
        const std::vector<RawResult> results = tester.run();
        tester.writeRawCsv(rawPath, results);
        tester.writeAveragedCsv(averagedPath, results);
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
