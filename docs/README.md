# Informe – Motores paralelos de Mancala (Kalah) y despliegue en Kubernetes

## Índice

| # | Archivo | Contenido |
|---|---------|-----------|
| 1 | [01-arquitectura.md](01-arquitectura.md) | Componentes, diagrama Mermaid, API REST, CORS |
| 2 | [02-motor.md](02-motor.md) | Reglas Kalah(6,4), Minimax + Alfa-Beta, pruebas |
| 3 | [03-paralelizacion.md](03-paralelizacion.md) | Estrategia OpenMP, métricas T(p) S(p) E(p), profiling |
| 4 | [04-despliegue-local.md](04-despliegue-local.md) | Docker Compose + Kubernetes local |
| 5 | [05-despliegue-nube.md](05-despliegue-nube.md) | Manifiestos YAML, proveedor cloud, evidencias |
| 6 | [06-cicd.md](06-cicd.md) | GitHub Actions workflows, SonarCloud |
| 7 | [07-analisis-comparativo.md](07-analisis-comparativo.md) | Local vs nube: latencia p50/p95, throughput |
| 8 | [08-conclusiones.md](08-conclusiones.md) | Limitaciones, retos, trabajo futuro |

## Mapeo a la rúbrica

| Criterio de la rúbrica | Archivo donde se evalúa |
|------------------------|------------------------|
| Motor de Mancala: corrección | [02-motor.md](02-motor.md) |
| Paralelización con OpenMP | [03-paralelizacion.md](03-paralelizacion.md) |
| Instrumentación local | [03-paralelizacion.md](03-paralelizacion.md) |
| Separación de componentes | [01-arquitectura.md](01-arquitectura.md) |
| Despliegue local | [04-despliegue-local.md](04-despliegue-local.md) |
| Despliegue en la nube con Kubernetes | [05-despliegue-nube.md](05-despliegue-nube.md) |
| CI/CD y calidad de código | [06-cicd.md](06-cicd.md) |
| Análisis comparativo local vs. nube | [07-analisis-comparativo.md](07-analisis-comparativo.md) |
| Claridad de explicaciones | Transversal a todos los archivos |
| Conclusiones | [08-conclusiones.md](08-conclusiones.md) |
