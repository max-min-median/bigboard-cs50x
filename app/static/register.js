const registrationForm = document.getElementById('registration-form');
const usernameInput = document.getElementById('username');
const usernameWarning = document.getElementById('username-whitespace-warning');
const passwordInput = document.getElementById('password');
const passwordWarning = document.getElementById('password-whitespace-warning');
const confirmationInput = document.getElementById('confirmation');
const matchWarning = document.getElementById('password-match-warning');

if (registrationForm) {

    function validateWhitespace(inputEl, warningEl) {
        const val = inputEl.value;
        const hasSpace = val.length > 0 && (val.startsWith(' ') || val.endsWith(' '));
        warningEl.style.display = hasSpace ? 'block' : 'none';
    }

    function checkMatch() {
        const mismatch = confirmationInput.value.length > 0 && passwordInput.value !== confirmationInput.value;
        matchWarning.style.display = mismatch ? 'block' : 'none';
        return !mismatch;
    }

    // Warn user about username containing leading or trailing whitespace
    usernameInput.addEventListener('input', function() {
        validateWhitespace(usernameInput, usernameWarning);
    });

    // Warn user about password containing leading or trailing whitespace
    passwordInput.addEventListener('input', function() {
        validateWhitespace(passwordInput, passwordWarning);
        checkMatch();
    });

    confirmationInput.addEventListener('input', checkMatch);

    registrationForm.addEventListener('submit', function(event) {
        if (!checkMatch()) {
            event.preventDefault();
        }
    });

} // end registrationForm guard
