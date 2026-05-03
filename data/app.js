/*
 * ============================================================================
 * 🔐 SESSÃO
 * ============================================================================
 */
let sessionToken = null;
let busy = false;
let statusInterval = null;

/*
 * ============================================================================
 * 🔐 LOGIN
 * ============================================================================
 */
async function login() {
    const pin = document.getElementById("pin").value;

    if (!pin) {
        alert("Digite o PIN");
        return;
    }

    try {
        const response = await fetch('/api/login', {
            method: 'POST',
            body: pin
        });

        if (!response.ok) {
            alert("PIN incorreto");
            return;
        }

        sessionToken = await response.text();

        updateUI("Autenticado");
        document.getElementById("loginBox").style.display = "none";

        startStatusPolling();

    } catch (err) {
        alert("Erro de conexão");
    }
}

/*
 * 🔐 LOGOUT AUTOMÁTICO
 */
function logout() {
    sessionToken = null;

    if (statusInterval) {
        clearInterval(statusInterval);
        statusInterval = null;
    }

    updateUI("Sessão expirada");
    document.getElementById("loginBox").style.display = "block";
}

/*
 * ============================================================================
 * 🌐 NAVEGAÇÃO
 * ============================================================================
 */
function go(page) {
    window.location.href = "/" + page;
}

function back() {
    window.location.href = "/";
}

/*
 * ============================================================================
 * 📡 COMANDO PROTEGIDO
 * ============================================================================
 */
async function sendCommand(endpoint) {

    if (!sessionToken) {
        alert("Faça login primeiro");
        return null;
    }

    try {
        const response = await fetch(endpoint, {
            method: 'GET',
            headers: {
                'Authorization': sessionToken
            }
        });

        /*
         * 🔥 Sessão expirou
         */
        if (response.status === 401) {
            logout();
            return null;
        }

        if (!response.ok) {
            updateUI("Erro HTTP: " + response.status);
            return null;
        }

        return await response.text();

    } catch (error) {
        updateUI("Falha de conexão");
        return null;
    }
}

/*
 * ============================================================================
 * 🎛️ CONTROLE DO MOTOR
 * ============================================================================
 */
async function motorCmd(endpoint) {

    if (busy) return;

    busy = true;
    disableButtons(true);
    updateUI("Enviando comando...");

    const res = await sendCommand(endpoint);

    if (!res) {
        busy = false;
        disableButtons(false);
        return;
    }

    if (res === "BUSY") {
        updateUI("Motor ocupado");
        busy = false;
        disableButtons(false);
        return;
    }

    updateUI("Executando...");
}

/*
 * 🛑 STOP
 */
async function stopMotor() {

    const res = await sendCommand('/api/stop');

    if (res) {
        updateUI("Parado manualmente");
    }

    busy = false;
    disableButtons(false);
}

/*
 * ============================================================================
 * 📊 STATUS EM TEMPO REAL (polling)
 * ============================================================================
 */
function startStatusPolling() {

    if (statusInterval) return;

    statusInterval = setInterval(async () => {

        if (!sessionToken) return;

        try {
            const response = await fetch('/api/status', {
                method: 'GET',
                headers: {
                    'Authorization': sessionToken
                }
            });

            if (response.status === 401) {
                logout();
                return;
            }

            if (!response.ok) return;

            const data = await response.json();

            if (data.status === "RUNNING") {
                updateUI("Executando...");
                disableButtons(true);
                busy = true;
            } else {
                updateUI("Parado");
                disableButtons(false);
                busy = false;
            }

        } catch (err) {
            updateUI("Sem conexão");
        }

    }, 500);
}

/*
 * ============================================================================
 * ➕ / ➖
 * ============================================================================
 */
function inc() {
    motorCmd('/api/inc');
}

function dec() {
    motorCmd('/api/dec');
}

/*
 * ============================================================================
 * 🎛️ UI HELPERS
 * ============================================================================
 */
function updateUI(msg) {
    const el = document.getElementById("status");
    if (el) {
        el.innerText = "Status: " + msg;
    }
}

function disableButtons(state) {
    const inc = document.getElementById("btnInc");
    const dec = document.getElementById("btnDec");
    const stop = document.getElementById("btnStop");

    if (inc) inc.disabled = state;
    if (dec) dec.disabled = state;

    /*
     * STOP:
     * só habilita se estiver rodando
     */
    if (stop) stop.disabled = !state;
}