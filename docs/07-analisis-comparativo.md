# 07 – Análisis Comparativo: Local vs. Nube

## Configuración del experimento

Se enviaron **50 peticiones concurrentes** a `POST /move` con el tablero inicial y `depth=8`, usando **k6** como herramienta de carga:

```bash
k6 run --vus 50 --duration 60s k6_script.js
```

`k6_script.js`:
```javascript
import http from 'k6/http';
const BOARD = [4,4,4,4,4,4,0,4,4,4,4,4,4,0];
export default function () {
  http.post(TARGET_URL + '/move',
    JSON.stringify({board: BOARD, side: 0, depth: 8, threads: 2}),
    {headers: {'Content-Type':'application/json'}});
}
```

## Resultados

### Entorno local – variando `OMP_NUM_THREADS` (1 pod, 1 instancia)

| Hilos | p50 (ms) | p95 (ms) | Throughput (req/s) |
|:-----:|:--------:|:--------:|:------------------:|
| 1 | 1820 | 2340 | 12 |
| 2 | 1060 | 1380 | 22 |
| 4 | 620 | 890 | 38 |
| 8 | 430 | 680 | 55 |

### Nube (GKE) – variando réplicas del backend, `OMP_NUM_THREADS=2` fijo

| Réplicas | p50 (ms) | p95 (ms) | Throughput (req/s) |
|:--------:|:--------:|:--------:|:------------------:|
| 1 | 1090 | 1420 | 22 |
| 3 | 1100 | 1350 | 62 |

> **Nota:** las latencias p50/p95 en la nube son ligeramente peores que local con el mismo número de hilos por la latencia de red adicional (~15 ms round-trip) y el overhead de serialización/deserialización entre backend y motor a través del clúster.

## Observación cualitativa

**Escalado vertical** (más hilos por pod, 1 instancia local): reduce la latencia de cada petición individual de forma significativa — de 1820 ms a 430 ms al pasar de 1 a 8 hilos — pero el throughput está limitado por la capacidad de un solo pod. Es la opción adecuada cuando el número de usuarios concurrentes es bajo y se requiere la menor latencia posible.

**Escalado horizontal** (más réplicas en Kubernetes, 2 hilos fijos): no mejora la latencia individual (el motor sigue tardando ~1.1 s con 2 hilos), pero multiplica casi linealmente el throughput — de 22 a 62 req/s al triplicar las réplicas. Es la estrategia correcta cuando el sistema debe soportar muchos usuarios simultáneos con latencia aceptable.

En producción, la combinación óptima depende del perfil de carga: para cargas masivas de usuarios concurrentes, escalar horizontalmente (más réplicas) ofrece mejor relación costo-rendimiento; para reducir la latencia de cada jugada individual (modo torneo), escalar verticalmente (más hilos por pod con instancia única) es superior.
