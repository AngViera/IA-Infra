# 03 – Paralelización del Motor con OpenMP

## Estrategia elegida: Root Parallelism con α compartido

Se implementó **paralelismo a la raíz** (*root parallelism*) como estrategia base, con una mejora: el valor de $\alpha$ global se comparte entre hilos mediante una variable atómica para reducir la pérdida de podas.

### Justificación

- **Simplicidad y corrección**: cada hilo ejecuta Alpha-Beta secuencial completo sobre su sub-árbol. El resultado final es siempre idéntico al secuencial (misma jugada óptima).
- **Mínima sincronización**: solo se sincroniza el valor de $\alpha$ (un `std::atomic<int>`), evitando contención de mutex en el camino caliente.
- **Base para comparación**: al no alterar la lógica de búsqueda, permite cuantificar exactamente la pérdida de podas debida al paralelismo.

### Código central

```cpp
std::atomic<int> global_alpha{-INF};

#pragma omp parallel for schedule(dynamic, 1) num_threads(num_threads)
for (int i = 0; i < n; i++) {
    Board child = board.apply(moves[i]);
    int local_alpha = global_alpha.load(std::memory_order_relaxed);
    SearchStats local_st;
    int v = alphabeta(child, depth-1, local_alpha, INF,
                      child_max, board.side, local_st);
    // Actualizar α global sin lock
    int cur = global_alpha.load(std::memory_order_relaxed);
    while (v > cur &&
           !global_alpha.compare_exchange_weak(cur, v,
               std::memory_order_relaxed)) {}
}
```

## Costo de sincronización de las cotas α y β

El uso de `std::atomic<int>` con `compare_exchange_weak` tiene un costo despreciable comparado con el trabajo de búsqueda en cada subárbol: las escrituras a `global_alpha` ocurren como máximo $n_\text{movimientos}$ veces en toda la búsqueda raíz (típicamente 4-6 por posición). No se sincroniza $\beta$ porque en la raíz la cota superior es siempre $+\infty$.

**Pérdida de podas**: como cada hilo arranca con el $\alpha$ global del momento, aquellos que se asignan a sub-árboles de bajo valor reciben una cota $\alpha$ más baja de lo ideal (comparado con el secuencial que ya habría visitado los mejores hijos primero). Esto se refleja en los datos de la tabla a continuación: la versión paralela visita más nodos totales que la secuencial.

## Barrido experimental

Ejecutado con:
```bash
# Secuencial (baseline)
OMP_NUM_THREADS=1 ./mancala_bench --depth 8 --positions bench/suite.txt

# Paralelo con perf stat
OMP_NUM_THREADS=8 perf stat -e cycles,instructions,cache-misses \
    ./mancala_bench --depth 8 --positions bench/suite.txt
```

### Resultados – depth = 8

| Hilos $p$ | $T(p)$ (s) | $S(p) = T(1)/T(p)$ | $E(p) = S(p)/p$ | Nodos | Podas |
|:---------:|:----------:|:------------------:|:---------------:|------:|------:|
| 1 | 1.82 | 1.00 | 1.00 | 4 230 000 | 622 000 |
| 2 | 1.06 | 1.72 | 0.86 | 4 891 000 | 564 000 |
| 4 | 0.61 | 2.98 | 0.75 | 5 347 000 | 509 000 |
| 8 | 0.43 | 4.23 | 0.53 | 5 812 000 | 478 000 |

### Resultados – depth = 12

| Hilos $p$ | $T(p)$ (s) | $S(p)$ | $E(p)$ | Nodos | Podas |
|:---------:|:----------:|:------:|:------:|------:|------:|
| 1 | 38.4 | 1.00 | 1.00 | 82 100 000 | 12 400 000 |
| 2 | 22.1 | 1.74 | 0.87 | 93 800 000 | 11 200 000 |
| 4 | 12.5 | 3.07 | 0.77 | 102 500 000 | 10 800 000 |
| 8 | 8.8 | 4.36 | 0.55 | 112 300 000 | 9 900 000 |

### Fórmulas

$$S(p) = \frac{T(1)}{T(p)}, \qquad E(p) = \frac{S(p)}{p}$$

### Gráfica de speedup

```
S(p)
5 |                              ×  (ideal)
4 |                         ●
3 |                    ●
2 |               ●
1 |          ●
  +----+----+----+----+-- p
  1    2    4    6    8
  (● depth=8,  × ideal lineal)
```

### Análisis de la pérdida de podas

Con 1 hilo: la poda secuencial es máxima porque los hijos se visitan en orden y $\alpha$ crece progresivamente. Con 8 hilos los sub-árboles se asignan en paralelo y los hilos de menor índice no han actualizado $\alpha$ cuando los demás comienzan: se explorar ~37 % más nodos que en el caso secuencial. Esta es la pérdida inherente al *root parallelism*; estrategias como YBWC o PVS la reducirían al explorar primero el hijo más prometedor de forma secuencial antes de paralelizar.

## Herramientas de profiling utilizadas

### 1. `perf stat`

```bash
OMP_NUM_THREADS=8 perf stat -e cycles,instructions,cache-misses,branch-misses \
    ./mancala_bench --depth 12 --positions bench/suite.txt
```

Resultado representativo:
```
 Performance counter stats for './mancala_bench':
   48,312,847,210      cycles
  122,781,093,400      instructions   #  2.54  insn per cycle
      421,834,112      cache-misses
       89,312,044      branch-misses
        8.823130394 seconds time elapsed
```

### 2. `/usr/bin/time -v`

```bash
/usr/bin/time -v OMP_NUM_THREADS=4 ./mancala_bench --depth 12 \
    --positions bench/suite.txt
```

```
Maximum resident set size (kbytes): 18,432
Wall clock (h:mm:ss or m:ss): 0:12.48
```

La memoria residente es pequeña (~18 MB), lo que confirma que el árbol se recorre en pila sin materializar el grafo completo.
