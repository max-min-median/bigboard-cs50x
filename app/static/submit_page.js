// csrfToken and scriptName are needed in multiple scripts, but only defined here since the .js gets bundled
const csrfToken = document.querySelector('meta[name="csrf-token"]').content;
const scriptName = document.querySelector('meta[name="script-name"]').content;

const submitForm = document.getElementById('submit-form');
const codeInput = document.getElementById('code-input');
const outputDiv = document.getElementById('output');
const submitBtn = document.getElementById('submit-btn');
const useDistCheckbox = document.getElementById('use-dist-checkbox');
const compatibilityMode = document.getElementById('compatibility-mode-checkbox');
const headerGroup = document.getElementById('header-group');
const headerInput = document.getElementById('header-input');
const dividerDiv = document.getElementById('textarea-divider');
const textareasContainer = document.getElementById('textareas-container');
const codeGroup = document.querySelector('.textarea-group');

// Since all .js bundled together, make sure this doesn't run on non-submission pages
if (codeGroup) {

// Push divider down so it aligns with the textareas, not the labels
const codeLabelRow = codeGroup.querySelector('.textarea-label-row');
const labelGap = parseFloat(getComputedStyle(codeGroup).gap) || 0;
dividerDiv.style.marginTop = (codeLabelRow.getBoundingClientRect().height + labelGap) + 'px';

// File upload handlers
function loadFileIntoTextarea(inputId, textarea) {
    const fileInput = document.getElementById(inputId);
    fileInput.addEventListener('change', function() {
        const file = fileInput.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = function(e) {
            textarea.value = e.target.result;
            textarea.dispatchEvent(new Event('input'));
        };
        reader.readAsText(file);
        fileInput.value = '';
    });
}
loadFileIntoTextarea('code-file-input', codeInput);
loadFileIntoTextarea('header-file-input', headerInput);

// Drag-and-drop into textareas
function enableFileDrop(textarea) {
    textarea.addEventListener('dragover', function(e) {
        e.preventDefault();
        textarea.classList.add('drag-over');
    });
    textarea.addEventListener('dragleave', function() {
        textarea.classList.remove('drag-over');
    });
    textarea.addEventListener('drop', function(e) {
        e.preventDefault();
        textarea.classList.remove('drag-over');
        const file = e.dataTransfer.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = function(ev) {
            textarea.value = ev.target.result;
            textarea.dispatchEvent(new Event('input'));
        };
        reader.readAsText(file);
    });
}
enableFileDrop(codeInput);
enableFileDrop(headerInput);

// Disable benchmark button when no code in textarea
codeInput.addEventListener('input', function() {
    const isLoggedIn = submitBtn.getAttribute('data-logged-in') === 'true';
    submitBtn.disabled = !isLoggedIn || codeInput.value.trim() === '';
});

// show or hide textarea for dictionary.h, reset settings when checked
useDistCheckbox.addEventListener('change', function() {
    const useDefault = useDistCheckbox.checked;
    
    headerGroup.hidden = useDefault;
    dividerDiv.hidden = useDefault;
    if (useDefault) {
        codeGroup.style.flexBasis = '';
        codeGroup.style.flexGrow = '';
        codeGroup.style.flexShrink = '';
    }
});

// implement drag handle for resizing the two textareas - kind of unnecessary but small QoL
dividerDiv.addEventListener('mousedown', function(e) {
    e.preventDefault();
    // prevent cursor flickering and text selection
    document.body.style.cursor = 'col-resize';
    document.body.style.userSelect = 'none';

    const containerRect = textareasContainer.getBoundingClientRect();
    const dividerWidth = dividerDiv.offsetWidth;
    const minWidth = 200;
    const maxLeft = containerRect.width - dividerWidth - minWidth;

    function onMouseMove(e) {
        const leftWidth = Math.max(minWidth, Math.min(maxLeft, e.clientX - containerRect.left));
        codeGroup.style.flexBasis = leftWidth + 'px';
        codeGroup.style.flexGrow = '0';
        codeGroup.style.flexShrink = '0';
    }

    function onMouseUp() {
        document.body.style.cursor = '';
        document.body.style.userSelect = '';
        document.removeEventListener('mousemove', onMouseMove);
        document.removeEventListener('mouseup', onMouseUp);
    }

    document.addEventListener('mousemove', onMouseMove);
    document.addEventListener('mouseup', onMouseUp);
});

// submit code in textareas to /submit route
submitForm.addEventListener('submit', async function(e) {
    e.preventDefault();

    submitBtn.disabled = true;
    codeInput.disabled = true;
    headerInput.disabled = true;
    outputDiv.textContent = 'Submitting...';

    let submissionId = null;
    try {
        const response = await fetch(scriptName + '/submit', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json', 'X-CSRFToken': csrfToken },
            body: JSON.stringify({
                code: codeInput.value,
                header: useDistCheckbox.checked ? '' : headerInput.value,
                compatibility: compatibilityMode.checked,
            })
        });
        const data = await response.json();
        if (!response.ok || !data.submission_id) {
            throw new Error(data.error || 'Submission failed.');
        }
        submissionId = data.submission_id;
    } catch (err) {
        outputDiv.textContent = 'Error: ' + err.message;
        submitBtn.disabled = false; // TODO group these lines together in a function, repeated a bunch
        codeInput.disabled = false;
        headerInput.disabled = false;
        return;
    }

    // Poll /status/<id> until done
    const pollInterval = setInterval(async function() {
        try {
            const res = await fetch(scriptName + '/status/' + submissionId);
            const data = await res.json();

            if (data.status === 'pending') {
                outputDiv.textContent = `${data.position} submission(s) ahead of you...`;
            } else if (data.status === 'compiling') {
                outputDiv.textContent = 'Compiling code...';
            } else if (data.status === 'running') {
                outputDiv.textContent = 'Running benchmarks...';
            } else if (data.status === 'done' || data.status === 'error') {
                clearInterval(pollInterval);
                outputDiv.textContent = data.output;
                submitBtn.disabled = false;
                codeInput.disabled = false;
                headerInput.disabled = false;
            }
        } catch (err) {
            clearInterval(pollInterval);
            outputDiv.textContent = 'Error: could not reach server.';
            submitBtn.disabled = false;
            codeInput.disabled = false;
            headerInput.disabled = false;
        }
    }, parseInt(submitBtn.dataset.pollInterval, 10));
});

} // end codeGroup guard
