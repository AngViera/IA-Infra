#pragma once
#include "kalah.h"
#include <atomic>
#include <algorithm>
#include <limits>
#include <omp.h>

// Valor "infinito" seguro (mitad del maximo int para evitar overflow al negar)
static constexpr int INF = std::numeric_limits<int>::max() / 2;

// Peso de las semillas en juego dentro de la heuristica
static constexpr double SIDE_WEIGHT = 0.15;

// Funcion heuristica:
// h = (semillas_kalaha_propio - semillas_kalaha_rival)
//   + SIDE_WEIGHT * (semillas_lado_propio - semillas_lado_rival)
inline int evaluate(const Board& b, int max_player) {
    int rival = 1 - max_player;
    int score = (b.pits[b.store(max_player)] - b.pits[b.store(rival)])
              + static_cast<int>(SIDE_WEIGHT * 100 *
                    (b.seeds_on_side(max_player) - b.seeds_on_side(rival)));
    return score;
}

// Evaluacion exacta cuando el juego termina
inline int terminal_eval(const Board& b, int max_player) {
    Board fin = b.collect_remaining();
    return fin.pits[fin.store(max_player)] - fin.pits[fin.store(1 - max_player)];
}

// Contadores para medir el rendimiento de la busqueda
struct SearchStats {
    long long nodes{0};
    long long prunes{0};
};

// Minimax sin poda (referencia para verificar correctitud de alfa-beta)
inline int minimax(const Board& b, int depth, bool is_max,
                   int max_player, SearchStats& st) {
    st.nodes++;
    if (depth == 0 || b.is_terminal())
        return b.is_terminal() ? terminal_eval(b, max_player)
                               : evaluate(b, max_player);

    auto moves = b.legal_moves();
    if (moves.empty()) return evaluate(b, max_player);

    if (is_max) {
        int best = -INF;
        for (int m : moves) {
            Board child = b.apply(m);
            bool child_max = (child.turn == max_player);
            int val = minimax(child, depth - 1, child_max, max_player, st);
            best = std::max(best, val);
        }
        return best;
    } else {
        int best = INF;
        for (int m : moves) {
            Board child = b.apply(m);
            bool child_max = (child.turn == max_player);
            int val = minimax(child, depth - 1, child_max, max_player, st);
            best = std::min(best, val);
        }
        return best;
    }
}

inline int minimax_best_move(const Board& b, int depth, SearchStats& st) {
    auto moves = b.legal_moves();
    int best_val = -INF, best_move = moves[0];
    for (int m : moves) {
        Board child = b.apply(m);
        bool child_max = (child.turn == b.turn);
        int val = minimax(child, depth - 1, child_max, b.turn, st);
        if (val > best_val) { best_val = val; best_move = m; }
    }
    return best_move;
}

// Alfa-Beta secuencial
inline int alphabeta(const Board& b, int depth, int alpha, int beta,
                     bool is_max, int max_player, SearchStats& st) {
    st.nodes++;
    if (depth == 0 || b.is_terminal())
        return b.is_terminal() ? terminal_eval(b, max_player)
                               : evaluate(b, max_player);

    auto moves = b.legal_moves();
    if (moves.empty()) return evaluate(b, max_player);

    if (is_max) {
        int val = -INF;
        for (int m : moves) {
            Board child = b.apply(m);
            bool child_max = (child.turn == max_player);
            val = std::max(val, alphabeta(child, depth - 1, alpha, beta,
                                          child_max, max_player, st));
            alpha = std::max(alpha, val);
            if (beta <= alpha) { st.prunes++; break; }
        }
        return val;
    } else {
        int val = INF;
        for (int m : moves) {
            Board child = b.apply(m);
            bool child_max = (child.turn == max_player);
            val = std::min(val, alphabeta(child, depth - 1, alpha, beta,
                                          child_max, max_player, st));
            beta = std::min(beta, val);
            if (beta <= alpha) { st.prunes++; break; }
        }
        return val;
    }
}

inline int alphabeta_best_move(const Board& b, int depth, SearchStats& st) {
    auto moves = b.legal_moves();
    int best_val = -INF, best_move = moves[0];
    int alpha = -INF, beta = INF;
    for (int m : moves) {
        Board child = b.apply(m);
        bool child_max = (child.turn == b.turn);
        int val = alphabeta(child, depth - 1, alpha, beta,
                            child_max, b.turn, st);
        if (val > best_val) { best_val = val; best_move = m; }
        alpha = std::max(alpha, best_val);
    }
    return best_move;
}

// Resultado del motor paralelo
struct ParallelResult { int move{-1}; int val{-INF}; };

// Alfa-Beta paralelo con paralelismo de raiz (OpenMP)
// Cada hilo explora un subárbol independiente desde la raíz
inline ParallelResult alphabeta_parallel(const Board& b, int depth,
                                         int num_threads, SearchStats& st) {
    auto moves = b.legal_moves();
    if (moves.empty()) return {-1, evaluate(b, b.turn)};

    int n = static_cast<int>(moves.size());
    std::vector<int> vals(n, -INF);
    std::vector<long long> t_nodes(n, 0), t_prunes(n, 0);

    // Alpha global compartido entre hilos (sin mutex, operacion atomica)
    std::atomic<int> global_alpha{-INF};

#pragma omp parallel for schedule(dynamic,1) num_threads(num_threads)
    for (int i = 0; i < n; i++) {
        Board child = b.apply(moves[i]);
        bool child_max = (child.turn == b.turn);
        int local_alpha = global_alpha.load(std::memory_order_relaxed);

        SearchStats local_st;
        int v = alphabeta(child, depth - 1, local_alpha, INF,
                          child_max, b.turn, local_st);
        vals[i]     = v;
        t_nodes[i]  = local_st.nodes;
        t_prunes[i] = local_st.prunes;

        // Actualizar alpha global con CAS (compare-and-swap)
        int cur = global_alpha.load(std::memory_order_relaxed);
        while (v > cur &&
               !global_alpha.compare_exchange_weak(cur, v,
                   std::memory_order_relaxed)) {}
    }

    for (int i = 0; i < n; i++) {
        st.nodes  += t_nodes[i];
        st.prunes += t_prunes[i];
    }

    int best_idx = static_cast<int>(
        std::max_element(vals.begin(), vals.end()) - vals.begin());
    return {moves[best_idx], vals[best_idx]};
}
