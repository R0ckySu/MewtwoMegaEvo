"""User registration/login backed by SQLite, with session-cookie auth."""
import re
import sqlite3
from contextlib import closing

from fastapi import APIRouter, Depends, HTTPException, Request
from passlib.context import CryptContext
from pydantic import BaseModel

from . import settings

router = APIRouter()
_pwd = CryptContext(schemes=["pbkdf2_sha256"], deprecated="auto")
_USERNAME_RE = re.compile(r"^[A-Za-z0-9_-]{3,32}$")


def init_db() -> None:
    with closing(sqlite3.connect(settings.USERS_DB)) as db:
        db.execute(
            "CREATE TABLE IF NOT EXISTS users ("
            "  username TEXT PRIMARY KEY,"
            "  pwd_hash TEXT NOT NULL,"
            "  created_at TEXT DEFAULT CURRENT_TIMESTAMP)"
        )
        db.commit()


def _get_hash(username: str):
    with closing(sqlite3.connect(settings.USERS_DB)) as db:
        row = db.execute(
            "SELECT pwd_hash FROM users WHERE username = ?", (username,)
        ).fetchone()
    return row[0] if row else None


class Credentials(BaseModel):
    username: str
    password: str


def get_current_user(request: Request) -> str:
    """Dependency: return the logged-in username or raise 401."""
    user = request.session.get("user")
    if not user:
        raise HTTPException(status_code=401, detail="Not authenticated")
    return user


@router.post("/register")
def register(creds: Credentials, request: Request):
    if not _USERNAME_RE.match(creds.username):
        raise HTTPException(
            status_code=400,
            detail="Username must be 3-32 chars: letters, digits, _ or -")
    if len(creds.password) < 6:
        raise HTTPException(status_code=400, detail="Password must be >= 6 characters")
    try:
        with closing(sqlite3.connect(settings.USERS_DB)) as db:
            db.execute(
                "INSERT INTO users (username, pwd_hash) VALUES (?, ?)",
                (creds.username, _pwd.hash(creds.password)))
            db.commit()
    except sqlite3.IntegrityError:
        raise HTTPException(status_code=409, detail="Username already taken")
    request.session["user"] = creds.username
    return {"username": creds.username}


@router.post("/login")
def login(creds: Credentials, request: Request):
    pwd_hash = _get_hash(creds.username)
    if not pwd_hash or not _pwd.verify(creds.password, pwd_hash):
        raise HTTPException(status_code=401, detail="Invalid username or password")
    request.session["user"] = creds.username
    return {"username": creds.username}


@router.post("/logout")
def logout(request: Request):
    request.session.clear()
    return {"ok": True}


@router.get("/me")
def me(user: str = Depends(get_current_user)):
    return {"username": user}
