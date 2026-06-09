# Mancala Kalah – Motor paralelo con Minimax y Alfa-Beta

**Asignatura**: Infraestructuras Paralelas y Distribuidas  
**Universidad**: Universidad del Valle  
**Profesor**: Carlos Andrés Delgado S, Msc

## Integrantes

| Nombre completo | Código | Correo institucional |
|-----------------|--------|----------------------|
| *Mariana Viera Serna* | *2569104* | *marianavieraserna@gmail.com* |

---

## Descripción

Motor de juego para **Mancala Kalah(6,4)** con búsqueda adversaria Minimax + poda Alfa-Beta, paralelizado con OpenMP y desplegado como microservicios en Docker/Kubernetes.

---

## Estructura del proyecto

```
.
├── motor/          # Motor C++/OpenMP – lógica del juego + algoritmos IA + servidor HTTP
├── backend/        # Wrapper Python/FastAPI
├── frontend/       # Interfaz web (HTML/JS + nginx)
├── deploy/
│   ├── local/      # Docker Compose + manifiestos Kubernetes locales
│   └── cloud/      # Manifiestos YAML para GKE/EKS/AKS
├── .github/
│   └── workflows/  # CI/CD con GitHub Actions + SonarCloud
└── docs/           # Informe (8 secciones en Markdown)
```

---

## Cómo ejecutar en GitHub Codespaces

Esta es la forma más sencilla de correr el proyecto completo sin instalar nada localmente.

### 1. Abrir el Codespace

1. Ir al repositorio en GitHub.
2. Hacer clic en el botón verde **Code**.
3. Seleccionar la pestaña **Codespaces**.
4. Hacer clic en **Create codespace on main**.

Esperar ~30 segundos mientras el entorno arranca (Ubuntu con Docker preinstalado).

### 2. Verificar Docker

En la terminal del Codespace:

```bash
docker --version
docker compose version
```

### 3. Levantar todos los servicios

```bash
cd deploy/local
docker compose up --build
```

La primera vez tarda ~3-5 minutos (compila el motor C++). Cuando aparezca:

```
motor     | Listening on 0.0.0.0:8080
backend   | Uvicorn running on http://0.0.0.0:8000
frontend  | nginx: ready
```

todo está corriendo.

### 4. Abrir la interfaz

En Codespaces, ir a la pestaña **Ports** (parte inferior del editor). Hacer clic en el globo del puerto **8090** para abrirlo en el navegador.

Si no aparece automáticamente, agregarlo manualmente con protocolo `http`.

### 5. Probar el API desde la terminal

```bash
# Verificar servicios
curl http://localhost:8000/healthz    # {"status":"ok"}
curl http://localhost:8080/healthz    # ok

# Solicitar un movimiento a la IA
curl -s -X POST http://localhost:8000/move \
  -H "Content-Type: application/json" \
  -d '{"board":[4,4,4,4,4,4,0,4,4,4,4,4,4,0],"side":0,"depth":7,"threads":4}' \
  | python3 -m json.tool
```

### 6. Detener

```bash
docker compose down
```

---

## Compilar solo el motor (sin Docker)

```bash
sudo apt-get update && sudo apt-get install -y cmake g++ libssl-dev libgomp1

cmake -B build -DCMAKE_BUILD_TYPE=Release motor/
cmake --build build --parallel

cd build
ctest --output-on-failure

OMP_NUM_THREADS=4 ./mancala_bench --depth 7 --positions motor/bench/suite.txt
```

---

## Despliegue Kubernetes local (kind)

```bash
docker build -t mancala-motor:local    motor/
docker build -t mancala-backend:local  backend/
docker build -t mancala-frontend:local frontend/

kind create cluster --name mancala
kind load docker-image mancala-motor:local    --name mancala
kind load docker-image mancala-backend:local  --name mancala
kind load docker-image mancala-frontend:local --name mancala

kubectl apply -f deploy/local/k8s/
kubectl get pods,svc,deploy
```

---

## Variables de entorno

| Variable | Servicio | Default | Descripción |
|----------|----------|---------|-------------|
| `OMP_NUM_THREADS` | motor | `4` | Hilos OpenMP |
| `MOTOR_URL` | backend | `http://motor-svc:8080` | URL interna del motor |
| `ALLOWED_ORIGINS` | backend | `http://localhost:8090` | Orígenes CORS permitidos |

---

## Informe completo

Ver [`docs/README.md`](docs/README.md).
