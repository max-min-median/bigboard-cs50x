const localDates = document.querySelectorAll('.local-date');
const editLabelBtns = document.querySelectorAll('.btn-edit-label');
const saveLabelBtns = document.querySelectorAll('.btn-save-label');
const cancelLabelBtns = document.querySelectorAll('.btn-cancel-label');
const deleteBtns = document.querySelectorAll('.btn-delete');
const labelInputs = document.querySelectorAll('.label-input');
const savedCountSpan = document.getElementById('saved-count');


// Format all .local-date spans from Unix timestamps to user's local time
localDates.forEach(el => {
    el.textContent = new Date(el.dataset.timestamp * 1000).toLocaleDateString(undefined, {
        year: '2-digit', month: '2-digit', day: '2-digit'
    });
});


editLabelBtns.forEach(btn => {
    btn.addEventListener('click', () => {
        const id = btn.dataset.id;
        const row = document.getElementById('row-' + id);
        row.querySelector('.label-text').classList.add('d-none');
        row.querySelector('.label-input').classList.remove('d-none');
        row.querySelector('.btn-edit-label').classList.add('d-none');
        row.querySelector('.btn-save-label').classList.remove('d-none');
        row.querySelector('.btn-cancel-label').classList.remove('d-none');
    });
});

// Cancel label edit: revert to original label text
cancelLabelBtns.forEach(btn => {
    btn.addEventListener('click', () => {
        const id = btn.dataset.id;
        const row = document.getElementById('row-' + id);
        const labelText = row.querySelector('.label-text');
        const labelInput = row.querySelector('.label-input');
        labelInput.value = labelText.textContent;
        labelInput.classList.add('d-none');
        labelText.classList.remove('d-none');
        row.querySelector('.btn-edit-label').classList.remove('d-none');
        row.querySelector('.btn-save-label').classList.add('d-none');
        btn.classList.add('d-none');
    });
});

// Save: POST new label to server, update text on success
saveLabelBtns.forEach(btn => {
    btn.addEventListener('click', async () => {
        const id = btn.dataset.id;
        const row = document.getElementById('row-' + id);
        const labelText = row.querySelector('.label-text');
        const labelInput = row.querySelector('.label-input');
        const newLabel = labelInput.value.trim();

        try {
            const res = await fetch(`/submission/${id}/label`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ label: newLabel })
            });
            if (!res.ok) throw new Error();
            labelText.textContent = newLabel;
        } catch {
            alert('Failed to save label. Please try again.');
            return;
        }

        labelInput.classList.add('d-none');
        labelText.classList.remove('d-none');
        row.querySelector('.btn-edit-label').classList.remove('d-none');
        btn.classList.add('d-none');
        row.querySelector('.btn-cancel-label').classList.add('d-none');
    });
});

// Enter/Esc in label input triggers save/cancel
labelInputs.forEach(input => {
    input.addEventListener('keydown', e => {
        const row = input.closest('tr');
        if (e.key === 'Enter') row.querySelector('.btn-save-label').click();
        if (e.key === 'Escape') row.querySelector('.btn-cancel-label').click();
    });
});

// Delete: remove submission row from db
deleteBtns.forEach(btn => {
    btn.addEventListener('click', async () => {
        if (!confirm('Delete this submission?')) return;

        const id = btn.dataset.id;
        try {
            const res = await fetch(`/submission/${id}/delete`, { method: 'POST' });
            if (!res.ok) throw new Error();
            document.getElementById('row-' + id).remove();
            const data = await res.json();
            savedCountSpan.textContent = data.saved_count;
        } catch {
            alert('Failed to delete submission. Please try again.');
        }
    });
});
