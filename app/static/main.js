const submitForm = document.getElementById('submit-form');
const codeInput = document.getElementById('code-input');
const outputDiv = document.getElementById('output');
const submitBtn = document.getElementById('submit-btn');
const useDistCheckbox = document.getElementById('use-dist-checkbox');
const headerGroup = document.getElementById('header-group');
const headerInput = document.getElementById('header-input');
const dividerDiv = document.getElementById('textarea-divider');
const textareasContainer = document.getElementById('textareas-container');
const codeGroup = document.querySelector('.textarea-group');

// Push divider down so it aligns with the textareas, not the labels
const codeLabel = codeGroup.querySelector('label');
const labelGap = parseFloat(getComputedStyle(codeGroup).gap) || 0;
dividerDiv.style.marginTop = (codeLabel.getBoundingClientRect().height + labelGap) + 'px';

// Disable benchmark button when no code in textarea
codeInput.addEventListener('input', function() {
    submitBtn.disabled = codeInput.value.trim() === '';
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
    outputDiv.textContent = 'Running...';

    try {
        const response = await fetch('/submit', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                code: codeInput.value,
                header: useDistCheckbox.checked ? '' : headerInput.value
            })
        });
        const data = await response.json();
        outputDiv.textContent = data.output;
    } catch (err) {
        outputDiv.textContent = 'Error: could not reach server.';
    } finally {
        submitBtn.disabled = false;
        codeInput.disabled = false;
        headerInput.disabled = false;
    }
});