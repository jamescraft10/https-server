let count = 0;

function button() {
    ++count;
    document.getElementById('button').innerText = `${count}`;
}