"""
FastAPI backend unit tests (uses httpx.AsyncClient with TestClient).
Run: pytest tests/
"""
import pytest
from fastapi.testclient import TestClient
from unittest.mock import AsyncMock, patch
import json

import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from app.main import app

client = TestClient(app)

INITIAL_BOARD = [4,4,4,4,4,4,0, 4,4,4,4,4,4,0]

MOTOR_OK_RESPONSE = {
    "move": 2,
    "evaluation": 1,
    "elapsed_ms": 120,
    "threads_used": 4,
    "stats": {"nodes": 100000, "prunes": 15000},
}


def mock_motor_post(url, **kwargs):
    class FakeResp:
        status_code = 200
        def json(self): return MOTOR_OK_RESPONSE
        text = json.dumps(MOTOR_OK_RESPONSE)
    return FakeResp()


# ── /healthz ──────────────────────────────────────────────────────────────────
def test_healthz():
    r = client.get("/healthz")
    assert r.status_code == 200
    assert r.json()["status"] == "ok"


# ── /move – happy path ────────────────────────────────────────────────────────
def test_move_success(monkeypatch):
    import httpx

    async def fake_post(self, url, **kwargs):
        class R:
            status_code = 200
            def json(self): return MOTOR_OK_RESPONSE
            text = json.dumps(MOTOR_OK_RESPONSE)
        return R()

    monkeypatch.setattr(httpx.AsyncClient, "post", fake_post)
    payload = {"board": INITIAL_BOARD, "side": 0, "depth": 6, "threads": 2}
    r = client.post("/move", json=payload)
    assert r.status_code == 200
    data = r.json()
    assert "move" in data
    assert "evaluation" in data
    assert "elapsed_ms" in data
    assert "stats" in data


# ── /move – validation ────────────────────────────────────────────────────────
def test_move_invalid_board_length():
    r = client.post("/move", json={"board": [4]*10, "side": 0, "depth": 6, "threads": 2})
    assert r.status_code == 422


def test_move_invalid_side():
    r = client.post("/move", json={"board": INITIAL_BOARD, "side": 2, "depth": 6, "threads": 2})
    assert r.status_code == 422


def test_move_negative_board_value():
    board = INITIAL_BOARD.copy()
    board[0] = -1
    r = client.post("/move", json={"board": board, "side": 0, "depth": 6, "threads": 2})
    assert r.status_code == 422


def test_move_depth_out_of_range():
    r = client.post("/move", json={"board": INITIAL_BOARD, "side": 0, "depth": 25, "threads": 2})
    assert r.status_code == 422


# ── /move – motor unavailable ─────────────────────────────────────────────────
def test_move_motor_unavailable(monkeypatch):
    import httpx

    async def fake_post(self, url, **kwargs):
        raise httpx.ConnectError("refused")

    monkeypatch.setattr(httpx.AsyncClient, "post", fake_post)
    r = client.post("/move", json={"board": INITIAL_BOARD, "side": 0, "depth": 6, "threads": 2})
    assert r.status_code == 503
