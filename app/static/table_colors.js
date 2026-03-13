const benchCells = document.querySelectorAll('tbody .bench-cell');

function lerp(a, b, t) { return a + (b - a) * t; }

// Group cells by column index, each column gets its own scale
const columns = {};
benchCells.forEach(cell => {
    (columns[cell.cellIndex] ??= []).push(cell);
});

Object.values(columns).forEach(cells => {
    const values = cells.map(c => parseFloat(c.textContent));
    const min = Math.min(...values);
    const max = Math.max(...values);

    cells.forEach((cell, i) => {
        const v = values[i];
        if (min === max || v === max) {
            cell.style.backgroundColor = 'rgba(13, 110, 253, 0.35)';
        } else {
            const t = (v - min) / (max - min);
            const r = Math.round(lerp(220, 25, t));
            const g = Math.round(lerp(53, 135, t));
            const b = Math.round(lerp(69, 84, t));
            cell.style.backgroundColor = `rgba(${r}, ${g}, ${b}, 0.35)`;
        }
    });
});
