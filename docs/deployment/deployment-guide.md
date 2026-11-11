# Deployment guide

## Architecture

```mermaid
flowchart LR
    User[Browser] --> Vercel[Vercel frontend]
    Vercel -->|HTTPS REST| Railway[Railway C++ backend]
    Railway --> Data[/data volume]
```

The live services are:

- Frontend: [frontend-amrit7.vercel.app](https://frontend-amrit7.vercel.app/)
- Backend health: [stratadb-production.up.railway.app/health](https://stratadb-production.up.railway.app/health)
- Backend records: [stratadb-production.up.railway.app/records](https://stratadb-production.up.railway.app/records)

## Railway backend

The [`Dockerfile`](../../Dockerfile) uses Debian Bookworm, installs build dependencies, configures CMake in Release mode, and builds `stratadb_server`. The container sets `HOST=0.0.0.0` and starts:

```text
./build/stratadb_server /data
```

Railway supplies `PORT`, which the server reads when no explicit port is supplied. Attach the Railway persistent volume at `/data`. The engine creates `/data/wal.log`, `/data/MANIFEST`, and `/data/sstables/` as needed. Do not reset, recreate, or delete this volume during deployment; these files are the database state.

## Vercel frontend

Deploy the `frontend` directory as a Vite project. Set the build environment variable:

```text
VITE_API_BASE_URL=https://stratadb-production.up.railway.app
```

The frontend calls the backend over HTTPS. No credentials or private keys belong in the repository. The current backend wildcard CORS policy permits the connection but should be narrowed to the Vercel origin before handling sensitive data.

## Deployment flow

1. Push source to GitHub.
2. Railway builds the Docker image and starts the C++ server against `/data`.
3. Vercel builds the frontend with `VITE_API_BASE_URL`.
4. Verify `/health`, `/records`, and the dashboard.
5. Confirm the attached volume remains mounted; persistence depends on it.

There is no repository `vercel.json` or Railway configuration file; hosting settings are configured in the respective platforms and the Dockerfile.
