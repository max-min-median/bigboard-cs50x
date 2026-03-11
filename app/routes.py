import logging

from flask import (
    Blueprint,
    jsonify,
    redirect,
    render_template,
    request,
    session,
)
from flask_login import current_user, login_required, login_user
from werkzeug.security import check_password_hash, generate_password_hash

from sqlalchemy import func
from sqlalchemy.orm import joinedload

from . import queue_worker
from .config import SUBMISSIONS_LEADERBOARD_ITEMS_PER_PAGE
from .models import Submission, User, db

log = logging.getLogger(__name__)

main = Blueprint("main", __name__)


@main.route("/")
@login_required
def index():
    return redirect("/submit")


@main.route("/login", methods=["POST", "GET"])
def login():
    """Log user in."""
    if request.method == "POST":
        # Forget any user_id
        session.clear()

        username = request.form.get("username").strip()
        password = request.form.get("password")

        # Validate whether username and password were submitted or not
        if not username or not password:
            return render_template(
                "error.html", message="Must provide username and password"
            )

        # Query database for user
        user = User.query.filter_by(username=username).first()

        # Check whether user exists and password is correct
        if not user or not check_password_hash(user.password_hash, password):
            return render_template(
                "error.html", message="Invalid username and/or password"
            )

        login_user(user)

        return redirect("/")

    else:
        return render_template("login.html")


@main.route("/logout")
@login_required
def logout():
    """Log user out."""
    session.clear()
    return redirect("/login")


@main.route("/register", methods=["POST", "GET"])
def register():
    """Register user."""
    if request.method == "POST":
        session.clear()

        username = request.form.get("username").strip()
        password = request.form.get("password")
        confirmation = request.form.get("confirmation")

        if not all([username, password, confirmation]):
            return render_template(
                "error.html", message="Must provide username, password, and/or confirmation"
            )

        if password != confirmation:
            return render_template(
                "error.html", message="Password and confirmation must match"
            )

        existing_user = User.query.filter_by(username=username).first()
        if existing_user:
            return render_template("error.html", message="Username already taken")

        password_hash = generate_password_hash(password)
        new_user = User(username=username, password_hash=password_hash)

        try:
            db.session.add(new_user)
            db.session.commit()
            login_user(new_user)

            return redirect("/")
        except Exception as _:
            db.session.rollback()
            return render_template("error.html", message="Database error")

    else:
        return render_template("register.html")


@main.route("/submit", methods=["POST", "GET"])
@login_required
def submit():
    """Let user submit code."""
    if request.method == "POST":
        data = request.get_json()
        if not data or "code" not in data:
            return jsonify({"error": "No code received."}), 400

        code = data["code"]
        header = data.get("header", "")

        item = queue_worker.enqueue(user_id=current_user.id, code=code, header=header)
        return jsonify({"submission_id": item.submission_id})

    else:
        return render_template("submit.html")


@main.route("/leaderboard")
def leaderboard():
    """Show leaderboard — one best submission per user, paginated."""
    page = max(1, request.args.get("page", 1, type=int))

    best_per_user = (
        db.session.query(
            Submission.user_id,
            func.max(Submission.total_average).label("best")
        )
        .group_by(Submission.user_id)
        .subquery()
    )

    # Join back to get the full submission row for each user's best
    query = (
        db.session.query(Submission)
        .options(joinedload(Submission.user))
        .join(
            best_per_user,
            db.and_(
                Submission.user_id == best_per_user.c.user_id,
                Submission.total_average == best_per_user.c.best,
            ),
        )
        .order_by(Submission.total_average)
    )

    pagination = query.paginate(page=page, per_page=SUBMISSIONS_LEADERBOARD_ITEMS_PER_PAGE, error_out=False)

    if page > pagination.pages > 0:
        return redirect(f"/leaderboard?page={pagination.pages}")

    return render_template(
        "leaderboard.html", 
        pagination=pagination, 
        per_page=SUBMISSIONS_LEADERBOARD_ITEMS_PER_PAGE
    )


@main.route("/status/<submission_id>")
def status(submission_id: str):
    """Return status of submission."""
    result = queue_worker.get_status(submission_id)
    if result is None:
        return jsonify({"error": "Unknown submission ID."}), 404
    return jsonify(result)
