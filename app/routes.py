import logging

from flask import (
    Blueprint,
    jsonify,
    redirect,
    render_template,
    request,
    session,
)
from flask_login import current_user, login_required, login_user, logout_user
from sqlalchemy import func
from sqlalchemy.orm import joinedload
from werkzeug.security import check_password_hash, generate_password_hash

from . import queue_worker
from .config import (
    SUBMISSIONS_LEADERBOARD_ITEMS_PER_PAGE, 
    SUBMISSIONS_MAX_PER_USER, 
    SUBMISSION_POLL_INTERVAL_MS
)
from .models import Submission, User, db

log = logging.getLogger(__name__)

main = Blueprint("main", __name__)


@main.route("/")
def index():
    return render_template("index.html", poll_interval=SUBMISSION_POLL_INTERVAL_MS)


@main.route("/login", methods=["POST", "GET"])
def login():
    if current_user.is_authenticated:
      return redirect("/")
    
    if request.method == "POST":
        # Forget any user_id
        session.clear()

        username = (request.form.get("username") or "").strip()
        password = request.form.get("password")

        # Validate whether username and password were submitted or not
        if not username or not password:
            return render_template("login.html", error="Must provide username and password.")

        # Query database for user
        user = db.session.execute(db.select(User).filter_by(username=username)).scalar_one_or_none()

        # Check whether user exists and password is correct
        if not user or not check_password_hash(user.password_hash, password):
            return render_template("login.html", error="Invalid username and/or password.")

        login_user(user)
        return redirect("/")

    else:
        return render_template("login.html")


@main.route("/logout")
@login_required
def logout():
    session.clear()
    logout_user()
    return redirect("/")


@main.route("/register", methods=["POST", "GET"])
def register():
    if current_user.is_authenticated:
        return redirect("/")
    
    if request.method == "POST":
        session.clear()

        username = (request.form.get("username") or "").strip()
        password = request.form.get("password")
        confirmation = request.form.get("confirmation")

        if not all((username, password, confirmation)):
            return render_template(
                "register.html",
                error="Must provide username, password, confirmation.",
                username=username,
            )

        if password != confirmation:
            return render_template(
                "register.html", error="Password and confirmation must match.", username=username
            )

        existing_user = db.session.execute(
            db.select(User).filter_by(username=username)).scalar_one_or_none()
        if existing_user:
            return render_template("register.html", error="Username already taken.", username=username)

        new_user = User(username=username, password_hash=generate_password_hash(password))

        try:
            db.session.add(new_user)
            db.session.commit()
            login_user(new_user)

            return redirect("/")
        except Exception as e:
            db.session.rollback()
            log.exception(e)
            return render_template(
                "register.html", error="Something went wrong, please try again.", username=username
            )

    else:
        return render_template("register.html")


@main.route("/submit", methods=["POST"])
@login_required
def submit():
    data = request.get_json()
    if not data or "code" not in data:
        return jsonify({"error": "No code received."}), 400

    code = data["code"]
    header = data.get("header", "")
    try:
        item = queue_worker.enqueue(user_id=current_user.id, code=code, header=header)
        return jsonify({"submission_id": item.submission_id})
    except Exception as e:
        log.exception(e)
        return jsonify({"error": "Unexpected server error, please try again shortly."}), 500


@main.route("/leaderboard")
def leaderboard():
    """Show leaderboard — one best submission per user, paginated."""
    page = max(1, request.args.get("page", 1, type=int))

    best_per_user = (
        db.session.query(
            Submission.user_id, func.max(Submission.total_average).label("best")
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
        .order_by(Submission.total_average.desc())
    )

    pagination = query.paginate(
        page=page, per_page=SUBMISSIONS_LEADERBOARD_ITEMS_PER_PAGE, error_out=False
    )

    if page > pagination.pages > 0:
        return redirect(f"/leaderboard?page={pagination.pages}")

    return render_template(
        "leaderboard.html",
        pagination=pagination,
        per_page=SUBMISSIONS_LEADERBOARD_ITEMS_PER_PAGE,
    )


@main.route("/profile")
@login_required
def profile():
    """Show current user's submissions."""
    submissions = db.session.scalars(
        db.select(Submission).
        filter_by(user_id=current_user.id).
        order_by(Submission.total_average.desc())
    ).all()
    return render_template(
        "profile.html",
        submissions=submissions,
        max_submissions=SUBMISSIONS_MAX_PER_USER,
    )


@main.route("/submission/<int:submission_id>/delete", methods=["POST"])
@login_required
def delete_submission(submission_id: int):
    submission = db.first_or_404(
        db.select(Submission).filter_by(id=submission_id, user_id=current_user.id)
    )
    try:
        db.session.delete(submission)
        db.session.commit()
        saved_count = db.session.scalar(
            db.select(func.count()).select_from(Submission).filter_by(user_id=current_user.id)
        )
        return jsonify({"ok": True, "saved_count": saved_count})
    except Exception as e:
        db.session.rollback()
        log.exception(e)
        return jsonify({"error": "Could not delete submission, please try again."}), 500


@main.route("/submission/<int:submission_id>/label", methods=["POST"])
@login_required
def edit_label(submission_id: int):
    submission = db.first_or_404(
        db.select(Submission).filter_by(id=submission_id, user_id=current_user.id)
    )
    data = request.get_json()
    if not data:
        return jsonify({"error": "No data received."}), 400
    try:
        submission.label = (data.get("label") or "")[:100]
        db.session.commit()
        return jsonify({"ok": True})
    except Exception as e:
        db.session.rollback()
        log.exception(e)
        return jsonify({"error": "Could not edit label data, please try again." }), 500
        

@main.route("/status/<submission_id>")
@login_required
def status(submission_id: str):
    result = queue_worker.get_status(submission_id)
    if result is None:
        return jsonify({"error": "Unknown submission ID."}), 404
    return jsonify(result)
