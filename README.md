# Mancala Kalah – Motores paralelos y despliegue en Kubernetes

**Curso**: Infraestructuras Paralelas y Distribuidas  
**Universidad**: Universidad del Valle  
**Profesor**: Carlos Andrés Delgado S, Msc

## Integrantes del grupo

| Nombre completo | Código estudiantil | Correo institucional |
|-----------------|-------------------|----------------------|
| *(Agregar nombre)* | *(Agregar código)* | *(Agregar correo)* |
| *(Agregar nombre)* | *(Agregar código)* | *(Agregar correo)* |

## Descripción

Motor de juego para **Mancala Kalah(6,4)** con búsqueda adversaria Minimax + poda Alfa-Beta, paralelizado con OpenMP y desplegado como microservicios en Kubernetes (local y nube).

## Estructura del repositorio

```
.
├── motor/          # Motor C++/OpenMP (Kalah engine + Alpha-Beta + servidor HTTP)
├── backend/        # Wrapper Python/FastAPI
├── frontend/       # Cliente web (HTML/JS + nginx)
├── deploy/
│   ├── local/      # Docker Compose + manifiestos kind/minikube
│   └── cloud/      # Manifiestos YAML para GKE/EKS/AKS
├── .github/
│   └── workflows/  # CI/CD GitHub Actions + SonarCloud
└── docs/           # Informe (8 archivos Markdown + índice)
```

## Cómo construir y ejecutar localmente

### Opción 1: Docker Compose (más sencillo)

```bash
cd deploy/local
docker compose up --build
# Abrir http://localhost:8090
```

### Opción 2: Kubernetes local con kind

```bash
# Prerequisitos: kind, kubectl, docker

# 1. Construir imágenes
docker build -t mancala-motor:local   motor/
docker build -t mancala-backend:local backend/
docker build -t mancala-frontend:local frontend/

# 2. Crear clúster
kind create cluster --name mancala

# 3. Cargar imágenes
kind load docker-image mancala-motor:local    --name mancala
kind load docker-image mancala-backend:local  --name mancala
kind load docker-image mancala-frontend:local --name mancala

# 4. Aplicar manifiestos
kubectl apply -f deploy/local/k8s/

# 5. Verificar
kubectl get pods,svc,deploy

# 6. Acceder (NodePort)
# Frontend: http://NODE_IP:30090
# API:      http://NODE_IP:30080
```

### Compilar el motor manualmente

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release motor/
cmake --build build --parallel
cd build && ctest                         # ejecutar tests
OMP_NUM_THREADS=4 ./mancala_bench \
    --depth 8 --positions motor/bench/suite.txt
```

## Informe

El informe completo se encuentra en [`docs/`](docs/README.md).

## Variables de entorno clave

| Variable | Componente | Default | Descripción |
|----------|-----------|---------|-------------|
| `OMP_NUM_THREADS` | motor | 4 | Hilos OpenMP |
| `MOTOR_URL` | backend | `http://motor-svc:8080` | URL interna del motor |
| `ALLOWED_ORIGINS` | backend | `http://localhost:8090` | Orígenes CORS permitidos |
