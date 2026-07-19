"""Portal-only authentication.

The lab portal (MEWTWO_PORTAL / portal_url) is the single account system:
a valid ``lab_session`` cookie with mewtwo access signs the browser in
transparently on its first API call; there is no local registration or
password login. The local users table only mirrors username -> email (from
the portal identity) so job-completion emails keep working from background
threads. userdata/<username>/ sandboxes are keyed by the portal username.
"""
import sqlite3
from contextlib import closing

from fastapi import APIRouter, Depends, HTTPException, Request
from fastapi.responses import HTMLResponse, RedirectResponse

from . import settings

router = APIRouter()
_portal_verifier = None


def _verifier():
    global _portal_verifier
    if _portal_verifier is None and settings.PORTAL_URL:
        from labportal_client import PortalVerifier

        _portal_verifier = PortalVerifier(settings.PORTAL_URL)
    return _portal_verifier


def init_db() -> None:
    with closing(sqlite3.connect(settings.USERS_DB)) as db:
        db.execute(
            "CREATE TABLE IF NOT EXISTS users ("
            "  username TEXT PRIMARY KEY,"
            "  email TEXT,"
            "  created_at TEXT DEFAULT CURRENT_TIMESTAMP)"
        )
        db.commit()


def _remember_user(username: str, email: str | None) -> None:
    with closing(sqlite3.connect(settings.USERS_DB)) as db:
        db.execute(
            "INSERT INTO users (username, email) VALUES (?, ?) "
            "ON CONFLICT(username) DO UPDATE SET email = excluded.email "
            "WHERE excluded.email IS NOT NULL AND excluded.email != ''",
            (username, email or None),
        )
        db.commit()


def get_email(username: str):
    """The portal-registered email for a user, or None."""
    with closing(sqlite3.connect(settings.USERS_DB)) as db:
        row = db.execute(
            "SELECT email FROM users WHERE username = ?", (username,)
        ).fetchone()
    return row[0] if row and row[0] else None


def _cookie_identity(request: Request):
    """The portal identity on this request (any service), or None."""
    verifier = _verifier()
    if verifier is None:
        return None
    return verifier.verify_cookie(request.cookies.get("lab_session"))


def get_current_user(request: Request) -> str:
    """Dependency: return the logged-in username or raise 401.

    A browser carrying a valid lab-portal cookie with mewtwo access is
    signed in transparently on its first API call."""
    user = request.session.get("user")
    if not user:
        identity = _cookie_identity(request)
        if (identity is not None and identity.preferred_username
                and identity.service_level("mewtwo") is not None):
            user = identity.preferred_username
            request.session["user"] = user
            _remember_user(user, identity.email)
    if not user:
        raise HTTPException(status_code=401, detail="Not authenticated")
    return user


@router.get("/sso-login")
def sso_login(request: Request):
    """Top-level SSO entry: bounce through the portal login and back."""
    verifier = _verifier()
    if verifier is None:
        return HTMLResponse(
            "<h2>Mewtwo: lab portal not configured</h2>"
            "<p>Set <code>MEWTWO_PORTAL</code> (or <code>portal_url</code> in the "
            "config YAML) to the portal base URL — all sign-ins go through it.</p>",
            status_code=503,
        )
    identity = _cookie_identity(request)
    if identity is None:
        return RedirectResponse(verifier.login_url(str(request.base_url)))
    if identity.service_level("mewtwo") is None:
        return HTMLResponse(
            "<h2>No Mewtwo access</h2>"
            f"<p>Signed in as <b>{identity.preferred_username}</b>, but your lab "
            "roles do not include the Mewtwo simulator. Ask a manager to update "
            f"your role tags, then <a href='{settings.PORTAL_URL}'>return to the "
            "portal</a>.</p>",
            status_code=403,
        )
    request.session["user"] = identity.preferred_username
    _remember_user(identity.preferred_username, identity.email)
    return RedirectResponse("/")


@router.post("/logout")
def logout(request: Request):
    request.session.clear()
    return {"ok": True, "portal_url": settings.PORTAL_URL}


@router.get("/me")
def me(request: Request, user: str = Depends(get_current_user)):
    return {"username": user, "email": get_email(user),
            "portal_url": settings.PORTAL_URL}
