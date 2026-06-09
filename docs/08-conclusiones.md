# 08 – Conclusiones

## Resultados obtenidos

El proyecto logró construir un motor de IA para Mancala Kalah(6,4) funcional y desplegable, con los siguientes logros medibles:

- **Corrección**: la suite de pruebas unitarias verifica que Alfa-Beta produce exactamente el mismo movimiento óptimo que Minimax puro a las mismas profundidades (depth=4 y depth=6), confirmando la correcta implementación de las cotas $\alpha$ y $\beta$.
- **Speedup real**: con 4 hilos OpenMP y depth=8 se obtuvo $S(4) = 2{,}98$ sobre la versión secuencial, con una eficiencia del 75 %. A 8 hilos el speedup alcanza 4,23 pero la eficiencia cae al 53 % debido a la pérdida de podas característica del *root parallelism*.
- **Escalado horizontal**: triplicar las réplicas del backend en Kubernetes elevó el throughput de 22 a 62 req/s manteniendo la latencia p95 por debajo de 1,4 s.

## Limitaciones encontradas

- **Pérdida de podas paralela**: el *root parallelism* sacrifica hasta un 37 % de la efectividad de las podas respecto al algoritmo secuencial. Estrategias como YBWC o PVS reducirían esta pérdida al explorar secuencialmente el primer hijo antes de paralelizar, pero son más complejas de implementar correctamente con OpenMP.
- **Motor de un solo nodo**: el contenedor del motor no se replica en Kubernetes (sería contraproducente sin estado compartido). Con una búsqueda distribuida real (p. ej., trabajo distribuido por niveles del árbol usando MPI) se podría escalar también el cómputo del motor.
- **Ausencia de tabla de transposición**: la heurística y la búsqueda no utilizan memoización. Añadir una tabla de transposición con Zobrist hashing podría reducir el espacio de búsqueda en un 30-50 % adicional.

## Retos y cómo se resolvieron

| Reto | Solución |
|------|----------|
| Corrección de la regla de turno extra en Alpha-Beta paralelo | Propagar `is_max = (child.side == maximizing_side)` en lugar de alternar booleano, cubriendo el turno extra de forma natural |
| CORS bloqueando peticiones del frontend al backend | Middleware CORSMiddleware con lista explícita de orígenes; inyectado desde ConfigMap para no hardcodear en la imagen |
| Sincronización de α entre hilos sin deadlock | `std::atomic<int>` con `compare_exchange_weak` en bucle; garantiza progreso sin bloqueos |
| Tiempo de compilación del motor en Docker (~3 min) | Build de múltiples etapas y caché de capas de CMake FetchContent |

## Recomendaciones de mejoras futuras

1. **Young Brothers Wait Concept (YBWC)**: implementar para reducir la pérdida de podas en la versión paralela y obtener speedups más cercanos al ideal.
2. **Tabla de transposición con Zobrist hashing**: reducir el árbol de búsqueda de forma significativa.
3. **Iterative Deepening**: combinar con Alpha-Beta para mejorar el ordenamiento de movimientos y optimizar la poda.
4. **Base de datos de persistencia**: agregar PostgreSQL con PersistentVolumeClaim para guardar historiales de partidas y rankings.
5. **Horizontal Pod Autoscaler (HPA)**: configurar autoescalado del backend basado en latencia p95 usando métricas personalizadas de Prometheus.

## Lecciones aprendidas

- La separación estricta de componentes en contenedores, aunque añade complejidad inicial (CORS, service discovery), hace el sistema sustancialmente más mantenible: cada pieza puede actualizarse, escalarse y depurarse de forma independiente.
- La poda Alfa-Beta y la paralelización tienen una tensión fundamental: más hilos = menos podas. Cuantificar esa tensión con métricas reales (nodos, podas, tiempo) es indispensable para tomar decisiones de diseño informadas.
- OpenMP es sorprendentemente eficiente para paralelismo a nivel de tarea cuando el trabajo por hilo es suficientemente grande (depth ≥ 8). Para profundidades menores, el overhead de creación de hilos supera la ganancia.
