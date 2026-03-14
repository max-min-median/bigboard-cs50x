const benchCells = document.querySelectorAll('tbody .bench-cell');

function lerp(a, b, t) { return a + (b - a) * t; }

// Group cells by column index, each column gets its own scale
const columns = {};
benchCells.forEach(cell => {
    (columns[cell.cellIndex] ??= []).push(cell);
});

Object.values(columns).forEach(cells => {
    const values = cells.map(cell => parseFloat(cell.textContent));
    const min = Math.min(...values);
    const max = Math.max(...values);

    cells.forEach((cell, i) => {
        const v = values[i];
        if (min === max || v === max) {
            // Blue background for best score in column
            cell.style.backgroundColor = 'rgba(10, 80, 255, 0.45)';
        } else {
            const t = (v - min) / (max - min);
            const r = Math.round(lerp(220, 20, t));
            const g = Math.round(lerp(50, 135, t));
            cell.style.backgroundColor = `rgba(${r}, ${g}, 75, 0.45)`;
        }
    });
});
