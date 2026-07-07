"""FastAPI application entry point.

Run with:  uvicorn webapp.main:app --reload --port 8000
"""
from fastapi import Depends, FastAPI
from fastapi.staticfiles import StaticFiles
from starlette.middleware.sessions import SessionMiddleware

from . import (auth, configs, files, noise, plots, projects, results, runner,
               schema, settings, system, workspaces)
from .auth import get_current_user

app = FastAPI(title="MewtwoMegaEvo Web")
app.add_middleware(SessionMiddleware, secret_key=settings.SESSION_SECRET,
                   same_site="lax")


@app.middleware("http")
async def _revalidate_static(request, call_next):
    """The SPA is served without a build step, so the browser must always pick up
    the latest app.js/styles.css/index.html. `no-cache` = revalidate every load;
    StaticFiles' ETag/Last-Modified still make that a cheap 304 when unchanged."""
    response = await call_next(request)
    if not request.url.path.startswith("/api"):
        response.headers["Cache-Control"] = "no-cache"
    return response


def _lan_ip():
    import socket
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))  # no packets sent; just picks the outbound iface
        ip = s.getsockname()[0]
        s.close()
        return ip
    except OSError:
        return None


@app.on_event("startup")
def _startup():
    auth.init_db()
    if settings.CONFIG_FILE_LOADED:
        print(f"[mewtwo-web] config file:  {settings.CONFIG_FILE_LOADED}", flush=True)
    print(f"[mewtwo-web] user data dir: {settings.USERDATA_DIR}", flush=True)
    print(f"[mewtwo-web] noise data:    {settings.NOISEDATA_DIR}", flush=True)
    print(f"[mewtwo-web] users db:      {settings.USERS_DB}", flush=True)
    print(f"[mewtwo-web] session secret: {settings.SESSION_SECRET_SOURCE}", flush=True)
    ip = _lan_ip()
    if ip:
        print(f"[mewtwo-web] LAN access:   http://{ip}:<port>  "
              f"(start uvicorn with --host 0.0.0.0 to allow it)", flush=True)


@app.get("/api/schema")
def get_schema(user: str = Depends(get_current_user)):
    return schema.get_schema()


app.include_router(auth.router, prefix="/api", tags=["auth"])
app.include_router(workspaces.router, prefix="/api", tags=["workspaces"])
app.include_router(configs.router, prefix="/api", tags=["configs"])
app.include_router(files.router, prefix="/api", tags=["files"])
app.include_router(runner.router, prefix="/api", tags=["runner"])
app.include_router(results.router, prefix="/api", tags=["results"])
app.include_router(projects.router, prefix="/api", tags=["projects"])
app.include_router(noise.router, prefix="/api", tags=["noise"])
app.include_router(plots.router, prefix="/api", tags=["plots"])
app.include_router(system.router, prefix="/api", tags=["system"])

# The zero-build SPA. Registered last so /api/* routes win.
app.mount("/", StaticFiles(directory=str(settings.WEBAPP_DIR / "static"), html=True),
          name="static")
