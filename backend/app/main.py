"""
Mancala Kalah – FastAPI backend wrapper
Delegates computation to the C++ motor container.
"""
import os
import time
import httpx
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field, field_validator
from typing import List
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# ── Configuration ─────────────────────────────────────────────────────────────
MOTOR_URL     = os.getenv("MOTOR_URL",     "http://motor-svc:8080")
ALLOWED_ORIGINS = os.getenv(
    "ALLOWED_ORIGINS",
    "http://localhost:8080,http://localhost:3000"
).split(",")

app = FastAPI(
    title="Mancala Kalah API",
    description="HTTP wrapper for the C++/OpenMP Minimax engine",
    version="1.0.0",
)

# ── CORS ──────────────────────────────────────────────────────────────────────
app.add_middleware(
    CORSMiddleware,
    allow_origins=ALLOWED_ORIGINS,
    allow_methods=["GET", "POST", "OPTIONS"],
    allow_headers=["Content-Type"],
)

# ── Schemas (pydantic v2) ──────────────────────────────────────────────────────
class MoveRequest(BaseModel):
    board: List[int] = Field(..., min_length=14, max_length=14,
                             description="14-int board: [p0_pits×6, kalaha0, p1_pits×6, kalaha1]")
    side: int        = Field(..., ge=0, le=1, description="Current player (0 or 1)")
    depth: int       = Field(8,  ge=1, le=20, description="Search depth")
    threads: int     = Field(4,  ge=1, le=64, description="OpenMP threads")

    @field_validator("board")
    @classmethod
    def board_non_negative(cls, v):
        if any(x < 0 for x in v):
            raise ValueError("All board values must be >= 0")
        return v


class Stats(BaseModel):
    nodes: int
    prunes: int


class MoveResponse(BaseModel):
    move: int
    evaluation: int
    elapsed_ms: int
    threads_used: int
    stats: Stats


# ── Endpoints ─────────────────────────────────────────────────────────────────
@app.post("/move", response_model=MoveResponse, status_code=200)
async def compute_move(req: MoveRequest):
    """Ask the motor for the best move given a board state."""
    payload = req.model_dump()
    try:
        async with httpx.AsyncClient(timeout=60.0) as client:
            resp = await client.post(f"{MOTOR_URL}/compute", json=payload)
    except httpx.ConnectError:
        raise HTTPException(status_code=503, detail="Motor service unavailable")
    except httpx.TimeoutException:
        raise HTTPException(status_code=504, detail="Motor timed out")

    if resp.status_code == 422:
        raise HTTPException(status_code=422, detail=resp.json().get("error", "Validation failed"))
    if resp.status_code != 200:
        raise HTTPException(status_code=500, detail=f"Motor error: {resp.text}")

    data = resp.json()
    return MoveResponse(
        move=data["move"],
        evaluation=data["evaluation"],
        elapsed_ms=data["elapsed_ms"],
        threads_used=data["threads_used"],
        stats=Stats(nodes=data["stats"]["nodes"], prunes=data["stats"]["prunes"]),
    )


@app.get("/healthz", status_code=200)
async def healthz():
    """Liveness probe – always 200 if process is alive."""
    return {"status": "ok"}


@app.get("/readyz", status_code=200)
async def readyz():
    """Readiness probe – 200 only if motor is reachable."""
    try:
        async with httpx.AsyncClient(timeout=3.0) as client:
            r = await client.get(f"{MOTOR_URL}/healthz")
        if r.status_code == 200:
            return {"status": "ready"}
    except Exception:
        pass
    raise HTTPException(status_code=503, detail="Motor not reachable")


@app.get("/metrics", status_code=200)
async def metrics():
    """Prometheus-format metrics forwarded from motor."""
    try:
        async with httpx.AsyncClient(timeout=5.0) as client:
            r = await client.get(f"{MOTOR_URL}/metrics")
        from fastapi.responses import PlainTextResponse
        return PlainTextResponse(r.text)
    except Exception as e:
        raise HTTPException(status_code=503, detail=str(e))
