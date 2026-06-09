#pragma once
#include <array>
#include <vector>
#include <stdexcept>

// Board layout (14 ints, canonical):
//  [0..5]  = Player-0 pits (left→right)
//  [6]     = Player-0 kalaha (store)
//  [7..12] = Player-1 pits  (right→left from Player-0 perspective)
//  [13]    = Player-1 kalaha (store)
//
// Anticlockwise order: 0→1→2→3→4→5→6→7→8→9→10→11→12→(skip 13 for P0)→0
//                      7→8→9→10→11→12→13→0→1→2→3→4→5→(skip 6 for P1)→7
//
// Opposite pits: pit i (0-5) ↔ pit (12-i), e.g. 0↔12, 5↔7

static constexpr int NUM_PITS   = 14;
static constexpr int KALAHA_P0  = 6;
static constexpr int KALAHA_P1  = 13;
static constexpr int SIDE_SIZE  = 6;

struct Board {
    std::array<int, NUM_PITS> pits{};
    int side{0};   // 0 = Player 0's turn, 1 = Player 1's turn

    // Standard Kalah(6,4) starting position
    static Board initial() {
        Board b;
        b.pits.fill(4);
        b.pits[KALAHA_P0] = 0;
        b.pits[KALAHA_P1] = 0;
        b.side = 0;
        return b;
    }

    // Construct from raw array
    static Board from_array(const std::array<int,14>& arr, int s) {
        Board b;
        b.pits = arr;
        b.side = s;
        return b;
    }

    int kalaha(int s) const { return s == 0 ? KALAHA_P0 : KALAHA_P1; }
    int pit_start(int s)   const { return s == 0 ? 0 : 7; }

    // Sum of seeds on player s's side (excluding kalaha)
    int seeds_on_side(int s) const {
        int start = pit_start(s), total = 0;
        for (int i = start; i < start + SIDE_SIZE; i++) total += pits[i];
        return total;
    }

    // Legal moves for current player (pits with seeds > 0)
    std::vector<int> legal_moves() const {
        std::vector<int> moves;
        int start = pit_start(side);
        for (int i = start; i < start + SIDE_SIZE; i++)
            if (pits[i] > 0) moves.push_back(i);
        return moves;
    }

    // Apply move; returns resulting board (side updated)
    Board apply(int pit) const {
        if (pits[pit] == 0)
            throw std::runtime_error("Empty pit selected");

        Board next = *this;
        int seeds = next.pits[pit];
        next.pits[pit] = 0;

        int skip = (side == 0) ? KALAHA_P1 : KALAHA_P0;
        int idx = pit;

        while (seeds > 0) {
            idx = (idx + 1) % NUM_PITS;
            if (idx == skip) idx = (idx + 1) % NUM_PITS;
            next.pits[idx]++;
            seeds--;
        }

        // Extra turn: last seed in own kalaha
        if (idx == kalaha(side)) {
            // side stays the same
            return next;
        }

        // Capture: last seed in empty own pit, opposite has seeds
        int own_start = pit_start(side);
        bool is_own_pit = (idx >= own_start && idx < own_start + SIDE_SIZE);
        if (is_own_pit && next.pits[idx] == 1) {
            int opp = 12 - idx;  // opposite pit index
            if (next.pits[opp] > 0) {
                next.pits[kalaha(side)] += next.pits[opp] + 1;
                next.pits[idx] = 0;
                next.pits[opp] = 0;
            }
        }

        next.side = 1 - side;
        return next;
    }

    // Terminal: one side is empty
    bool is_terminal() const {
        return seeds_on_side(0) == 0 || seeds_on_side(1) == 0;
    }

    // Collect remaining seeds into respective kalahas (call when terminal)
    Board collect_remaining() const {
        Board b = *this;
        for (int s = 0; s < 2; s++) {
            int start = pit_start(s);
            for (int i = start; i < start + SIDE_SIZE; i++) {
                b.pits[kalaha(s)] += b.pits[i];
                b.pits[i] = 0;
            }
        }
        return b;
    }

    // Winner: 0 = P0 wins, 1 = P1 wins, -1 = draw (only valid after collect_remaining)
    int winner() const {
        if (pits[KALAHA_P0] > pits[KALAHA_P1]) return 0;
        if (pits[KALAHA_P1] > pits[KALAHA_P0]) return 1;
        return -1;
    }
};
