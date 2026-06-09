# 02 – Motor de Mancala: Kalah(6,4)

## Reglas implementadas (Kalah(6,4))

| Regla | Implementación |
|-------|---------------|
| Tablero | 2 filas × 6 hoyos, 4 semillas iniciales cada hoyo, 2 kalahas en 0 |
| Distribución | Sentido antihorario; incluye kalaha propio, salta kalaha rival |
| Turno extra | Si la última semilla cae en el kalaha propio, el mismo jugador repite |
| Captura | Última semilla en hoyo propio vacío + hoyo opuesto con semillas → ambos al kalaha propio |
| Fin del juego | Un lado completamente vacío; las semillas restantes van al kalaha del dueño |
| Ganador | Mayor número de semillas en kalaha |

### Representación del tablero

```
Jugador 1: [12][11][10][ 9][ 8][ 7]   kalaha[13]
Jugador 0:  [0] [1] [2] [3] [4] [5]   kalaha[ 6]
```

El opuesto de `pit[i]` es `pit[12-i]` (válido para i ∈ 0–5 y i ∈ 7–12).

## Función de evaluación heurística

$$h(\text{estado}) = \bigl(k_{\text{propio}} - k_{\text{rival}}\bigr) + \alpha \cdot \bigl(s_{\text{propio}} - s_{\text{rival}}\bigr)$$

donde $k$ son semillas en el kalaha, $s$ semillas en el lado activo y $\alpha = 0{,}1$. El término $\alpha$ favorece ganar control del tablero además de acumular en el kalaha.

## Pseudocódigo: Minimax con poda Alfa-Beta

```
función alphabeta(nodo, profundidad, α, β, maximizando):
    si profundidad = 0 o nodo.terminal():
        devolver evaluar(nodo)

    movimientos ← nodo.movimientos_legales()
    si maximizando:
        v ← -∞
        para cada m en movimientos:
            hijo ← nodo.aplicar(m)
            v ← max(v, alphabeta(hijo, prof-1, α, β, hijo.lado = nodo.lado_max))
            α ← max(α, v)
            si β ≤ α: romper  # poda β
        devolver v
    si no:
        v ← +∞
        para cada m en movimientos:
            hijo ← nodo.aplicar(m)
            v ← min(v, alphabeta(hijo, prof-1, α, β, hijo.lado = nodo.lado_max))
            β ← min(β, v)
            si β ≤ α: romper  # poda α
        devolver v
```

### Manejo del turno extra

Cuando la última semilla cae en el kalaha propio, el lado del tablero no cambia (`child.side == parent.side`). El indicador `is_max` se pasa como `child.side == maximizing_side`, lo cual propaga correctamente el turno extra sin necesidad de lógica adicional.

## Suite de pruebas unitarias

Las pruebas se encuentran en `motor/tests/test_kalah.cpp` y usan **doctest** (single-header).

### Cómo ejecutarlas

```bash
# Desde la raíz del repositorio
cmake -B build -DCMAKE_BUILD_TYPE=Release motor/
cmake --build build --parallel
cd build && ctest --output-on-failure
```

### Casos cubiertos

| Test | Descripción |
|------|-------------|
| `Initial board is Kalah(6,4)` | Verifica 4 semillas × 12 hoyos, kalahas en 0 |
| `Legal moves at start` | 6 movimientos disponibles al inicio |
| `Sowing pit 2 from initial` | Distribución correcta de 4 semillas |
| `Sowing skips opponent kalaha` | Jugador 1 salta el kalaha de Jugador 0 |
| `Extra turn` | Última semilla en kalaha propio → mismo jugador |
| `Capture` | Captura cuando último hoyo propio estaba vacío |
| `Terminal detection` | Detecta correctamente fin de partida |
| `collect_remaining` | Semillas residuales van a los kalahas correctos |
| `Alpha-Beta == Minimax depth=4` | Mismo movimiento óptimo, menos nodos |
| `Alpha-Beta == Minimax depth=6` | Verificación en posición de medio juego |
| `Parallel == Sequential depth=6` | Versión paralela produce el mismo movimiento |
| `Alpha-Beta prunes > 0` | Confirma que la poda ocurre |
| `Heuristic favors more kalaha seeds` | Heurística ordena correctamente las posiciones |

### Evidencia de equivalencia Alfa-Beta = Minimax

La prueba `Alpha-Beta best move matches Minimax at depth 6` compara ambos algoritmos sobre la posición inicial y una posición de medio juego. En todos los casos el movimiento elegido es idéntico y `st_ab.nodes < st_mm.nodes`, demostrando que la poda no altera el resultado óptimo.
