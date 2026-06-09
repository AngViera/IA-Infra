#include "kalah.h"
#include "engine.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT 0
#include "httplib.h"
#include "json.hpp"
#include <iostream>
#include <chrono>
#include <atomic>

using json = nlohmann::json;

// ─── Global metrics (for /metrics endpoint) ──────────────────────────────
static std::atomic<long long> g_total_requests{0};
static std::atomic<long long> g_total_nodes{0};
static std::atomic<long long> g_total_prunes{0};
static std::atomic<long long> g_total_elapsed_ms{0};

// ─── Helpers ─────────────────────────────────────────────────────────────
static bool validate_board(const json& j, std::string& err) {
    if (!j.contains("board") || !j["board"].is_array() || j["board"].size() != 14) {
        err = "board must be an array of 14 integers"; return false;
    }
    for (auto& v : j["board"]) if (!v.is_number_integer()) {
        err = "board values must be integers"; return false;
    }
    if (!j.contains("side") || !j["side"].is_number_integer() ||
        (j["side"] != 0 && j["side"] != 1)) {
        err = "side must be 0 or 1"; return false;
    }
    if (!j.contains("depth") || !j["depth"].is_number_integer() ||
        j["depth"] < 1 || j["depth"] > 20) {
        err = "depth must be integer in [1, 20]"; return false;
    }
    if (!j.contains("threads") || !j["threads"].is_number_integer() ||
        j["threads"] < 1 || j["threads"] > 64) {
        err = "threads must be integer in [1, 64]"; return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) port = std::stoi(argv[1]);

    httplib::Server svr;

    // ── POST /compute ─────────────────────────────────────────────────────
    svr.Post("/compute", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Content-Type", "application/json; charset=utf-8");

        json body;
        try { body = json::parse(req.body); }
        catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"invalid JSON"})", "application/json");
            return;
        }

        std::string err;
        if (!validate_board(body, err)) {
            res.status = 422;
            json e; e["error"] = err;
            res.set_content(e.dump(), "application/json");
            return;
        }

        std::array<int,14> arr;
        for (int i = 0; i < 14; i++) arr[i] = body["board"][i].get<int>();
        int side    = body["side"].get<int>();
        int depth   = body["depth"].get<int>();
        int threads = body["threads"].get<int>();

        Board board = Board::from_array(arr, side);
        if (board.is_terminal()) {
            res.status = 400;
            res.set_content(R"({"error":"terminal board position"})", "application/json");
            return;
        }

        auto moves = board.legal_moves();
        if (moves.empty()) {
            res.status = 400;
            res.set_content(R"({"error":"no legal moves"})", "application/json");
            return;
        }

        SearchStats st;
        auto t0 = std::chrono::steady_clock::now();

        int best_move;
        int eval_val;

        if (threads == 1) {
            best_move = alphabeta_best_move(board, depth, st);
            // recompute eval for response
            SearchStats tmp;
            Board child = board.apply(best_move);
            bool cm = (child.side == board.side);
            eval_val = alphabeta(child, depth-1, -INF, INF, cm, board.side, tmp);
        } else {
            auto result = alphabeta_parallel(board, depth, threads, st);
            best_move = result.move;
            eval_val  = result.val;
        }

        auto t1 = std::chrono::steady_clock::now();
        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count();

        // Update global metrics
        g_total_requests++;
        g_total_nodes   += st.nodes;
        g_total_prunes  += st.prunes;
        g_total_elapsed_ms += elapsed;

        json resp;
        resp["move"]         = best_move;
        resp["evaluation"]   = eval_val;
        resp["elapsed_ms"]   = elapsed;
        resp["threads_used"] = threads;
        resp["stats"]["nodes"]  = st.nodes;
        resp["stats"]["prunes"] = st.prunes;

        res.set_content(resp.dump(), "application/json");
    });

    // ── GET /healthz ──────────────────────────────────────────────────────
    svr.Get("/healthz", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok"})", "application/json");
    });

    // ── GET /metrics ──────────────────────────────────────────────────────
    svr.Get("/metrics", [](const httplib::Request&, httplib::Response& res) {
        std::string body =
            "# HELP mancala_requests_total Total compute requests\n"
            "# TYPE mancala_requests_total counter\n"
            "mancala_requests_total " + std::to_string(g_total_requests.load()) + "\n"
            "# HELP mancala_nodes_total Total nodes explored\n"
            "# TYPE mancala_nodes_total counter\n"
            "mancala_nodes_total " + std::to_string(g_total_nodes.load()) + "\n"
            "# HELP mancala_prunes_total Total alpha-beta prunes\n"
            "# TYPE mancala_prunes_total counter\n"
            "mancala_prunes_total " + std::to_string(g_total_prunes.load()) + "\n"
            "# HELP mancala_elapsed_ms_total Total elapsed compute ms\n"
            "# TYPE mancala_elapsed_ms_total counter\n"
            "mancala_elapsed_ms_total " + std::to_string(g_total_elapsed_ms.load()) + "\n";
        res.set_content(body, "text/plain");
    });

    svr.set_exception_handler([](const httplib::Request&, httplib::Response& res,
                                  std::exception_ptr ep) {
        try { if (ep) std::rethrow_exception(ep); }
        catch (const std::exception& e) {
            res.status = 500;
            json err; err["error"] = e.what();
            res.set_content(err.dump(), "application/json");
        }
    });

    std::cout << "[motor] Listening on port " << port << std::endl;
    svr.listen("0.0.0.0", port);
    return 0;
}
