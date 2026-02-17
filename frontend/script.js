// Mock data for initial UI
let resources = [10, 10, 10, 10, 10];
let processes = [];
let pidCounter = 1;
let messages = [];
let semaphores = [
    { id: 0, value: 1 },
    { id: 1, value: 1 },
    { id: 2, value: 1 },
    { id: 3, value: 1 },
    { id: 4, value: 1 }
];

function renderResourceTable() {
    let html = '<table><tr><th>Resource</th>';
    for (let i = 0; i < resources.length; i++) html += `<th>R${i}</th>`;
    html += '</tr><tr><td>Available</td>';
    for (let i = 0; i < resources.length; i++) html += `<td>${resources[i]}</td>`;
    html += '</tr></table>';
    document.getElementById('resource-table').innerHTML = html;
}

function renderProcessTable() {
    // Automatically set the process with the lowest priority value to RUNNING
    if (processes.length > 0) {
        let minPriority = Math.min(...processes.map(p => p.priority));
        let runningSet = false;
        for (let p of processes) {
            if (!runningSet && p.priority === minPriority) {
                p.state = 'RUNNING';
                runningSet = true;
            } else {
                p.state = 'READY';
            }
        }
    }
    let html = '<table><tr><th>PID</th>';
    for (let i = 0; i < resources.length; i++) html += `<th>Allocated R${i}</th>`;
    for (let i = 0; i < resources.length; i++) html += `<th>Requested R${i}</th>`;
    html += '<th>State</th><th>Priority</th><th>Actions</th></tr>';
    for (let p of processes) {
        html += `<tr><td>${p.pid}</td>`;
        for (let i = 0; i < resources.length; i++) html += `<td>${p.allocated[i]}</td>`;
        for (let i = 0; i < resources.length; i++) html += `<td>${p.requested[i]}</td>`;
        html += `<td>${p.state || 'READY'}</td><td>${p.priority || 1}</td>`;
        html += `<td><button onclick="requestRes(${p.pid})">Request</button> <button onclick="releaseRes(${p.pid})">Release</button> <button onclick="removeProcess(${p.pid})">Remove</button></td></tr>`;
    }
    html += '</table>';
    document.getElementById('process-table').innerHTML = html;
}

function renderDeadlockStatus(status) {
    const el = document.getElementById('deadlock-status');
    if (status === 'deadlock') {
        el.textContent = 'Deadlock detected!';
        el.className = 'deadlock';
    } else if (status === 'safe') {
        el.textContent = 'System is in a safe state.';
        el.className = 'safe';
    } else {
        el.textContent = '';
        el.className = '';
    }
}

function renderMessages() {
    let html = '<h3>Messages</h3>';
    if (messages.length === 0) html += '<p>No messages.</p>';
    else {
        html += '<ul>';
        for (let m of messages) {
            html += `<li>From PID ${m.from} to PID ${m.to}: ${m.text}</li>`;
        }
        html += '</ul>';
    }
    document.getElementById('messages').innerHTML = html;
}

function renderSemaphores() {
    let html = '<h3>Semaphores</h3>';
    html += '<table><tr><th>ID</th><th>Value</th></tr>';
    for (let s of semaphores) {
        html += `<tr><td>${s.id}</td><td>${s.value}</td></tr>`;
    }
    html += '</table>';
    document.getElementById('semaphores').innerHTML = html;
}

// Process management
function addProcess() {
    const priority = prompt('Enter process priority (0=high, 1=norm, 2=low):', '1');
    processes.push({
        pid: pidCounter++,
        allocated: [0, 0, 0, 0, 0],
        requested: [0, 0, 0, 0, 0],
        state: 'READY',
        priority: parseInt(priority) || 1
    });
    renderProcessTable();
}
function removeProcess(pid) {
    processes = processes.filter(p => p.pid !== pid);
    renderProcessTable();
}
function forkProcess() {
    const pid = prompt('Enter PID to fork:');
    const proc = processes.find(p => p.pid == pid);
    if (!proc) return alert('Process not found!');
    processes.push({
        pid: pidCounter++,
        allocated: [...proc.allocated],
        requested: [...proc.requested],
        state: 'READY',
        priority: proc.priority
    });
    renderProcessTable();
}
function killProcess() {
    const pid = prompt('Enter PID to kill:');
    removeProcess(parseInt(pid));
}
function exitProcess() {
    const pid = prompt('Enter PID to exit:');
    removeProcess(parseInt(pid));
}
function quantum() {
    alert('Quantum expired! (Simulated)');
}

// IPC
function sendMsg() {
    const from = prompt('Sender PID:');
    const to = prompt('Receiver PID:');
    const text = prompt('Message:');
    messages.push({ from, to, text });
    renderMessages();
}
function receiveMsg() {
    alert('Receive message (simulated).');
}
function replyMsg() {
    alert('Reply message (simulated).');
}

// Semaphores
function newSemaphore() {
    const id = prompt('Semaphore ID (0-4):');
    const value = prompt('Initial value:');
    semaphores.push({ id: parseInt(id), value: parseInt(value) });
    renderSemaphores();
}
function semP() {
    const id = prompt('Semaphore ID:');
    const sem = semaphores.find(s => s.id == id);
    if (sem) {
        sem.value--;
        renderSemaphores();
    }
}
function semV() {
    const id = prompt('Semaphore ID:');
    const sem = semaphores.find(s => s.id == id);
    if (sem) {
        sem.value++;
        renderSemaphores();
    }
}

// Resource management
function addResource() {
    for (let i = 0; i < resources.length; i++) resources[i]++;
    renderResourceTable();
}
function requestRes(pid) {
    const req = prompt('Enter resource request as comma-separated values (e.g. 1,0,2,0,0):');
    if (!req) return;
    const arr = req.split(',').map(x => parseInt(x.trim()));
    if (arr.length !== resources.length) return alert('Invalid input!');
    const proc = processes.find(p => p.pid === pid);
    let canGrant = arr.every((v, i) => v <= resources[i]);
    if (!canGrant) {
        alert('Request denied (not enough resources available).');
        return;
    }
    for (let i = 0; i < resources.length; i++) {
        resources[i] -= arr[i];
        proc.allocated[i] += arr[i];
        proc.requested[i] = 0;
    }
    renderResourceTable();
    renderProcessTable();
}
function requestResource() {
    const pid = prompt('Enter PID to request resources:');
    requestRes(parseInt(pid));
}
function releaseRes(pid) {
    const rel = prompt('Enter resources to release as comma-separated values (e.g. 1,0,0,0,0):');
    if (!rel) return;
    const arr = rel.split(',').map(x => parseInt(x.trim()));
    if (arr.length !== resources.length) return alert('Invalid input!');
    const proc = processes.find(p => p.pid === pid);
    for (let i = 0; i < resources.length; i++) {
        let releaseAmt = Math.min(arr[i], proc.allocated[i]);
        proc.allocated[i] -= releaseAmt;
        resources[i] += releaseAmt;
    }
    renderResourceTable();
    renderProcessTable();
}
function releaseResource() {
    const pid = prompt('Enter PID to release resources:');
    releaseRes(parseInt(pid));
}

// Info
function procInfo() {
    const pid = prompt('Enter PID for process info:');
    const proc = processes.find(p => p.pid == pid);
    if (!proc) return alert('Process not found!');
    alert(JSON.stringify(proc, null, 2));
}
function totalInfo() {
    alert('Total Info (simulated):\n' + JSON.stringify(processes, null, 2));
}

function detectDeadlock() {
    // Simulate deadlock detection: if any process requests more than available, show deadlock
    let deadlock = false;
    for (let p of processes) {
        for (let i = 0; i < resources.length; i++) {
            if (p.requested[i] > resources[i]) deadlock = true;
        }
    }
    renderDeadlockStatus(deadlock ? 'deadlock' : 'safe');
}

document.getElementById('add-process').onclick = addProcess;
document.getElementById('fork-process').onclick = forkProcess;
document.getElementById('kill-process').onclick = killProcess;
document.getElementById('exit-process').onclick = exitProcess;
document.getElementById('quantum').onclick = quantum;
document.getElementById('send-msg').onclick = sendMsg;
document.getElementById('receive-msg').onclick = receiveMsg;
document.getElementById('reply-msg').onclick = replyMsg;
document.getElementById('new-semaphore').onclick = newSemaphore;
document.getElementById('sem-p').onclick = semP;
document.getElementById('sem-v').onclick = semV;
document.getElementById('add-resource').onclick = addResource;
document.getElementById('request-resource').onclick = requestResource;
document.getElementById('release-resource').onclick = releaseResource;
document.getElementById('proc-info').onclick = procInfo;
document.getElementById('total-info').onclick = totalInfo;
document.getElementById('detect-deadlock').onclick = detectDeadlock;

renderResourceTable();
renderProcessTable();
renderDeadlockStatus();
renderMessages();
renderSemaphores(); 