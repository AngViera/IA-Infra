#include "../src/kalah.h"
#include "../src/engine.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <omp.h>

struct Position {
    std::string id;
    Board board;
    int depth;
};

static std::vector<Position> load_suite(const std::string& path) {
    std::ifstream f(path);
    if (!f) { std::cerr << "Cannot open " << path << "\n"; exit(1); }
    std::vector<Position> suite;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        Position p;
        ss >> p.id;
        std::array<int,14> arr;
        for (auto& v : arr) ss >> v;
        int side, depth;
        ss >> side >> depth;
        p.board = Board::from_array(arr, side);
        p.depth = depth;
        suite.push_back(p);
    }
    return suite;
}

int main(int argc, char* argv[]) {
    std::string suite_path = "bench/suite.txt";
    int threads = 1;
    int depth_override = -1;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--positions" && i+1 < argc) suite_path = argv[++i];
        else if (arg == "--depth"   && i+1 < argc) depth_override = std::stoi(argv[++i]);
        else if (arg == "--threads" && i+1 < argc) threads = std::stoi(argv[++i]);
    }

    auto suite = load_suite(suite_path);
    std::cout << "Loaded " << suite.size() << " positions. threads=" << threads << "\n\n";

    long long total_nodes = 0, total_prunes = 0;
    double total_time = 0.0;

    for (auto& pos : suite) {
        int d = depth_override > 0 ? depth_override : pos.depth;
        if (pos.board.is_terminal()) {
            std::cout << pos.id << ": terminal, skip\n";
            continue;
        }

        SearchStats st;
        double t0 = omp_get_wtime();
        int move;
        if (threads == 1) {
            move = alphabeta_best_move(pos.board, d, st);
        } else {
            auto r = alphabeta_parallel(pos.board, d, threads, st);
            move = r.move;
        }
        double elapsed = omp_get_wtime() - t0;

        total_nodes  += st.nodes;
        total_prunes += st.prunes;
        total_time   += elapsed;

        std::cout << pos.id
                  << "  depth=" << d
                  << "  move=" << move
                  << "  nodes=" << st.nodes
                  << "  prunes=" << st.prunes
                  << "  time=" << elapsed << "s\n";
    }

    std::cout << "\nTOTAL nodes=" << total_nodes
              << "  prunes=" << total_prunes
              << "  time=" << total_time << "s\n";
    return 0;
}
