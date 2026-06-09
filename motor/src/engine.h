#pragma once
#include "kalah.h"
#include <atomic>
#include <algorithm>
#include <limits>
#include <omp.h>

static constexpr int INF = std::numeric_limits<int>::max() / 2;
static constexpr double ALPHA_WEIGHT = 0.1;

// ─── Heuristic ────────────────────────────────────────────────────────────────
// h = (kalaha_own - kalaha_rival) + α·(seeds_own - seeds_rival)
inline int evaluate(const Board& b, int maximizing_side) {
    const int rival = 1 - maximizing_side;
    int score = (b.pits[b.kalaha(maximizing_side)] - b.pits[b.kalaha(rival)])
              + static_cast<int>(ALPHA_WEIGHT * 100 *
                    (b.seeds_on_side(maximizing_side) - b.seeds_on_side(rival)));
    return score;
}

// Terminal evaluation (collect remaining seeds first)
inline int terminal_eval(const Board& b, int maximizing_side) {
    Board fin = b.collect_remaining();
    return fin.pits[fin.kalaha(maximizing_side)] - fin.pits[fin.kalaha(1 - maximizing_side)];
}

// ─── Stats ────────────────────────────────────────────────────────────────────
struct SearchStats {
    long long nodes{0};
    long long prunes{0};
};

// ─── Pure Minimax (no pruning) ─────────────────────────────────────────────
// Used to verify Alpha-Beta correctness on test positions.
inline int minimax(const Board& b, int depth, bool is_max,
                   int maximizing_side, SearchStats& st) {
    st.nodes++;
    if (depth == 0 || b.is_terminal())
        return b.is_terminal() ? terminal_eval(b, maximizing_side)
                               : evaluate(b, maximizing_side);

    auto moves = b.legal_moves();
    if (moves.empty()) return evaluate(b, maximizing_side);

    if (is_max) {
        int best = -INF;
        for (int m : moves) {
            Board child = b.apply(m);
            // If extra turn, same player maximizes; else opponent minimizes
            bool child_max = (child.side == maximizing_side);
            int val = minimax(child, depth - 1, child_max, maximizing_side, st);
            best = std::max(best, val);
        }
        return best;
    } else {
        int best = INF;
        for (int m : moves) {
            Board child = b.apply(m);
            bool child_max = (child.side == maximizing_side);
            int val = minimax(child, depth - 1, child_max, maximizing_side, st);
            best = std::min(best, val);
        }
        return best;
    }
}

// Best move from pure Minimax
inline int minimax_best_move(const Board& b, int depth, SearchStats& st) {
    auto moves = b.legal_moves();
    int best_val = -INF, best_move = moves[0];
    for (int m : moves) {
        Board child = b.apply(m);
        bool child_max = (child.side == b.side);
        int val = minimax(child, depth - 1, child_max, b.side, st);
        if (val > best_val) { best_val = val; best_move = m; }
    }
    return best_move;
}

// ─── Alpha-Beta (sequential) ──────────────────────────────────────────────
inline int alphabeta(const Board& b, int depth, int alpha, int beta,
                     bool is_max, int maximizing_side, SearchStats& st) {
    st.nodes++;
    if (depth == 0 || b.is_terminal())
        return b.is_terminal() ? terminal_eval(b, maximizing_side)
                               : evaluate(b, maximizing_side);

    auto moves = b.legal_moves();
    if (moves.empty()) return evaluate(b, maximizing_side);

    if (is_max) {
        int val = -INF;
        for (int m : moves) {
            Board child = b.apply(m);
            bool child_max = (child.side == maximizing_side);
            val = std::max(val, alphabeta(child, depth-1, alpha, beta,
                                          child_max, maximizing_side, st));
            alpha = std::max(alpha, val);
            if (beta <= alpha) { st.prunes++; break; }
        }
        return val;
    } else {
        int val = INF;
        for (int m : moves) {
            Board child = b.apply(m);
            bool child_max = (child.side == maximizing_side);
            val = std::min(val, alphabeta(child, depth-1, alpha, beta,
                                          child_max, maximizing_side, st));
            beta = std::min(beta, val);
            if (beta <= alpha) { st.prunes++; break; }
        }
        return val;
    }
}

// Best move from sequential Alpha-Beta
inline int alphabeta_best_move(const Board& b, int depth, SearchStats& st) {
    auto moves = b.legal_moves();
    int best_val = -INF, best_move = moves[0];
    int alpha = -INF, beta = INF;
    for (int m : moves) {
        Board child = b.apply(m);
        bool child_max = (child.side == b.side);
        int val = alphabeta(child, depth-1, alpha, beta,
                            child_max, b.side, st);
        if (val > best_val) { best_val = val; best_move = m; }
        alpha = std::max(alpha, best_val);
    }
    return best_move;
}

// ─── Parallel Alpha-Beta – Root Parallelism (OpenMP) ─────────────────────
// Each thread runs independent sequential Alpha-Beta on its assigned subtree.
// Threads share a global alpha via atomic to improve pruning.
struct ParallelResult { int move{-1}; int val{-INF}; };

inline ParallelResult alphabeta_parallel(const Board& b, int depth,
                                         int num_threads, SearchStats& st) {
    auto moves = b.legal_moves();
    if (moves.empty()) return {-1, evaluate(b, b.side)};

    int n = static_cast<int>(moves.size());

    // Per-thread results and stats
    std::vector<int>  vals(n, -INF);
    std::vector<long long> t_nodes(n, 0), t_prunes(n, 0);

    // Shared global lower bound (atomic for lock-free update)
    std::atomic<int> global_alpha{-INF};

#pragma omp parallel for schedule(dynamic,1) num_threads(num_threads)
    for (int i = 0; i < n; i++) {
        Board child = b.apply(moves[i]);
        bool child_max = (child.side == b.side);
        int local_alpha = global_alpha.load(std::memory_order_relaxed);

        SearchStats local_st;
        int v = alphabeta(child, depth - 1, local_alpha, INF,
                          child_max, b.side, local_st);
        vals[i]     = v;
        t_nodes[i]  = local_st.nodes;
        t_prunes[i] = local_st.prunes;

        // Update shared alpha
        int cur = global_alpha.load(std::memory_order_relaxed);
        while (v > cur &&
               !global_alpha.compare_exchange_weak(cur, v,
                   std::memory_order_relaxed)) {}
    }

    // Aggregate stats
    for (int i = 0; i < n; i++) {
        st.nodes  += t_nodes[i];
        st.prunes += t_prunes[i];
    }

    // Best move
    int best_idx = static_cast<int>(
        std::max_element(vals.begin(), vals.end()) - vals.begin());
    return {moves[best_idx], vals[best_idx]};
}
