#pragma once
#include <array>
#include <vector>
#include <stdexcept>

// Tablero de 14 posiciones:
// [0..5]  = hoyos del Jugador 0
// [6]     = kalaha del Jugador 0
// [7..12] = hoyos del Jugador 1
// [13]    = kalaha del Jugador 1
// El hoyo opuesto de i (0-5) es (12-i)

static constexpr int TOTAL_SLOTS  = 14;
static constexpr int STORE_P0     = 6;
static constexpr int STORE_P1     = 13;
static constexpr int PITS_PER_SIDE = 6;

struct Board {
    std::array<int, TOTAL_SLOTS> pits{};
    int turn{0};  // 0 = turno del Jugador 0, 1 = turno del Jugador 1

    static Board initial() {
        Board b;
        b.pits.fill(4);
        b.pits[STORE_P0] = 0;
        b.pits[STORE_P1] = 0;
        b.turn = 0;
        return b;
    }

    static Board from_array(const std::array<int,14>& arr, int t) {
        Board b;
        b.pits = arr;
        b.turn = t;
        return b;
    }

    int store(int player) const { return player == 0 ? STORE_P0 : STORE_P1; }
    int first_pit(int player) const { return player == 0 ? 0 : 7; }

    int seeds_on_side(int player) const {
        int start = first_pit(player), total = 0;
        for (int i = start; i < start + PITS_PER_SIDE; i++)
            total += pits[i];
        return total;
    }

    std::vector<int> legal_moves() const {
        std::vector<int> moves;
        int start = first_pit(turn);
        for (int i = start; i < start + PITS_PER_SIDE; i++)
            if (pits[i] > 0) moves.push_back(i);
        return moves;
    }

    Board apply(int pit) const {
        if (pits[pit] == 0)
            throw std::runtime_error("Hoyo vacio");

        Board next = *this;
        int seeds = next.pits[pit];
        next.pits[pit] = 0;

        // El kalaha del rival se omite durante la siembra
        int rival_store = (turn == 0) ? STORE_P1 : STORE_P0;
        int idx = pit;

        while (seeds > 0) {
            idx = (idx + 1) % TOTAL_SLOTS;
            if (idx == rival_store)
                idx = (idx + 1) % TOTAL_SLOTS;
            next.pits[idx]++;
            seeds--;
        }

        // Turno extra si la ultima semilla cayo en el propio kalaha
        if (idx == store(turn))
            return next;

        // Captura si la ultima semilla cayo en un hoyo propio que estaba vacio
        int start = first_pit(turn);
        bool is_own = (idx >= start && idx < start + PITS_PER_SIDE);
        if (is_own && next.pits[idx] == 1) {
            int opp = 12 - idx;
            if (next.pits[opp] > 0) {
                next.pits[store(turn)] += next.pits[opp] + 1;
                next.pits[idx] = 0;
                next.pits[opp] = 0;
            }
        }

        next.turn = 1 - turn;
        return next;
    }

    bool is_terminal() const {
        return seeds_on_side(0) == 0 || seeds_on_side(1) == 0;
    }

    Board collect_remaining() const {
        Board b = *this;
        for (int p = 0; p < 2; p++) {
            int start = first_pit(p);
            for (int i = start; i < start + PITS_PER_SIDE; i++) {
                b.pits[store(p)] += b.pits[i];
                b.pits[i] = 0;
            }
        }
        return b;
    }

    int winner() const {
        if (pits[STORE_P0] > pits[STORE_P1]) return 0;
        if (pits[STORE_P1] > pits[STORE_P0]) return 1;
        return -1;
    }
};
