    #include <cmath>

    #include "strategy.hpp"

    constexpr inline double INV_64 = 1.0/64.0;
    constexpr inline double epsilon = 1e-9;

    struct alignas(64) state {
        double mids[64] = {0.0};
        double sum = 0.0;
        double sq_sum = 0.0;
        uint32_t count = 0;
        uint32_t head = 0;
        int32_t position = 0; // {-1,0,1}
    };

    std::vector<csot::Order> orders;

    class spec_strategy final : public csot::Strategy {
        public:
            void on_init() {
                orders.reserve(1);
            }

            [[gnu::hot]]
            std::vector<csot::Order> on_tick(const csot::Tick& tick) {
                orders.clear();
                
                const int32_t slot = sti(tick.symbol);
                state& s = sym_states[slot];

                const double mid = (tick.bid_px + tick.ask_px) * 0.5;
                const double old_mid = s.mids[s.head];
                s.mids[s.head++] = mid;
                s.head &= 63;
                if (s.count < 64) [[unlikely]] {
                    s.count++;
                    s.sum += mid;
                    s.sq_sum += mid*mid;
                    if (s.count < 64) [[likely]] {
                        return orders;
                    }
                }
                else {
                    s.sum = s.sum - old_mid + mid;
                    s.sq_sum = s.sq_sum - old_mid*old_mid + mid*mid;
                }

                const double mean = s.sum*INV_64;
                const double var = s.sq_sum*INV_64 - mean*mean;
                const double diff2 = (mid - mean)*(mid - mean);

                if (var < epsilon) [[unlikely]] return orders;
                

                if (s.position == 0) {
                    if (diff2 >= 4.0*var) [[unlikely]] {
                        if (mid > mean) {
                            orders.emplace_back(csot::Order::Side::SELL, tick.symbol, tick.bid_px, 1);
                        }
                        else {
                            orders.emplace_back(csot::Order::Side::BUY, tick.symbol, tick.ask_px, 1);
                        }
                    }
                    return orders;
                }
                else if (diff2 <= 0.25*var) {
                    if (s.position == 1) {
                        orders.emplace_back(csot::Order::Side::SELL, tick.symbol, tick.bid_px, 1);
                    }
                    else {
                        orders.emplace_back(csot::Order::Side::BUY, tick.symbol, tick.ask_px, 1);
                    }
                    return orders;
                }
                return orders;
            }

            void on_fill(const csot::Order& o, [[maybe_unused]] double fill_price, uint32_t fill_qty) {
                const int32_t slot = sti(o.symbol);
                if (o.side == csot::Order::Side::BUY) {
                    sym_states[slot].position += fill_qty;
                }
                else {
                    sym_states[slot].position -= fill_qty;
                }
            }

        private:
            int32_t symbolc = 0;
            state sym_states[64];
            std::string_view symbols[64] = {""};
            inline int32_t sti(const std::string_view& s) {
                for(int32_t i=0; i<symbolc; i++) {
                    if (symbols[i] == s) [[likely]] {
                        return i;
                    }
                }
                const int32_t slot = symbolc++;
                symbols[slot] = s;
                return slot;
            }
    };

    extern "C" csot::Strategy* create_strategy() {
        return new spec_strategy();
    }