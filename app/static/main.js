const submitForm = document.getElementById('submit-form');
const codeInput = document.getElementById('code-input');
const output = document.getElementById('output');
const submitBtn = document.getElementById('submit-btn');

codeInput.addEventListener('input', function() {
    submitBtn.disabled = codeInput.value.trim() === '';
});

submitForm.addEventListener('submit', async function(e) {
    e.preventDefault();

    submitBtn.disabled = true;
    codeInput.disabled = true;
    output.textContent = 'Running...';

    try {
        const response = await fetch('/submit', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ code: codeInput.value })
        });
        const data = await response.json();
        output.textContent = data.output;
    } catch (err) {
        output.textContent = 'Error: could not reach server.';
    } finally {
        submitBtn.disabled = false;
        codeInput.disabled = false;
    }
});
