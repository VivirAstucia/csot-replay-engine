#include <sstream>
#include <fstream>
#include <chrono>
#include <x86intrin.h>

#include "engine.hpp"

namespace csot {
    Engine::Engine() = default; 

    template<typename T>
    inline const char* parse_value(const char *ptr, const char *end, T& value) {
        auto [nptr, _] = std::from_chars(ptr, end, value);
        if (nptr < end && *nptr == ',') {
            nptr++;
        }
        return nptr;
    }

    void Engine::load_ticks(std::string csv_path) {
        std::ifstream file(csv_path, std::ios::binary | std::ios::ate);
        if (!file) {
            std::cerr << "Can't open csv: " << csv_path << std::endl;
            return;
        }

        int64_t chr_cnt = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(chr_cnt);
        if (!file.read(buffer.data(), chr_cnt)) {
            std::cerr << "Can't read csv: " << csv_path << std::endl;
            return;
        }
        
        const char *ptr = buffer.data();
        const char *end = buffer.data() + chr_cnt;

        while (ptr<end && *ptr != '\n') ptr++;
        ptr++;

        // ticks.reserve(1000000); // Or one can use file size to estimate the number of ticks.

        Tick tick;
        while (ptr < end) {
            ptr = parse_value(ptr, end, tick.timestamp_ns);

            const char *start = ptr;
            tick.symbol = DEFAULT_SYMBOLS[start[3] - '0'];
            while (*ptr!=',')ptr++;
            ptr++;

            ptr = parse_value(ptr, end, tick.bid_px);
            ptr = parse_value(ptr, end, tick.ask_px);
            ptr = parse_value(ptr, end, tick.bid_qty);
            ptr = parse_value(ptr, end, tick.ask_qty);

            while (ptr<end && (*ptr == '\n' || *ptr=='\r')) ptr++;

            ticks.emplace_back(tick);
        }
    }

    void Engine::run(Strategy& strategy) {
        strategy.on_init();
        for (const Tick& tick : ticks) {
            _mm_lfence(); const uint32_t t1 = __rdtsc();
            std::vector<Order> orders = strategy.on_tick(tick);
            _mm_lfence(); const uint32_t t2 = __rdtsc();
            const uint32_t latency = t2 - t1;
            latency_histogram.record(latency);

            for (const Order& order : orders) {
                strategy.on_fill(order, order.price, order.qty);
            }
        }
    }

    void Engine::print_histogram() const {
        latency_histogram.print(std::cout);
    }
}