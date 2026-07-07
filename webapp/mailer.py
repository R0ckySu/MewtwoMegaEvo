"""Minimal SMTP mailer for run-completion reports.

A no-op unless SMTP is configured (see settings). Sending is best-effort and
never raises into the caller; failures are logged. Use send_async() from hot
paths so a slow SMTP server can't block the request/worker thread.
"""
import smtplib
import ssl
import threading
from email.message import EmailMessage

from . import settings


def is_configured() -> bool:
    return bool(settings.SMTP_HOST and settings.MAIL_FROM)


def send(to: str, subject: str, body: str) -> bool:
    if not is_configured() or not to:
        return False
    msg = EmailMessage()
    msg["From"] = settings.MAIL_FROM
    msg["To"] = to
    msg["Subject"] = subject
    msg.set_content(body)
    try:
        if settings.SMTP_SSL:
            ctx = ssl.create_default_context()
            with smtplib.SMTP_SSL(settings.SMTP_HOST, settings.SMTP_PORT,
                                  timeout=20, context=ctx) as s:
                if settings.SMTP_USER:
                    s.login(settings.SMTP_USER, settings.SMTP_PASSWORD)
                s.send_message(msg)
        else:
            with smtplib.SMTP(settings.SMTP_HOST, settings.SMTP_PORT, timeout=20) as s:
                if settings.SMTP_STARTTLS:
                    s.starttls(context=ssl.create_default_context())
                if settings.SMTP_USER:
                    s.login(settings.SMTP_USER, settings.SMTP_PASSWORD)
                s.send_message(msg)
        return True
    except Exception as e:  # network/auth/TLS — log and move on
        print(f"[mailer] failed to send to {to}: {e}")
        return False


def send_async(to: str, subject: str, body: str) -> None:
    threading.Thread(target=send, args=(to, subject, body), daemon=True).start()
