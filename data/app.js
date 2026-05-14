/*
 * ============================================================================
 * 💉 FRONTEND - CONTROLE DA SERINGA
 * ============================================================================
 *
 * Responsabilidades:
 *  - autenticar usuário via PIN
 *  - manter token persistente no navegador
 *  - enviar comandos protegidos ao ESP32
 *  - atualizar status em tempo real
 *  - controlar botões da interface
 *  - carregar/salvar calibragem no ESP32/NVS
 * ============================================================================
 */

const AppState = {
    token: localStorage.getItem("seringa_token"),
    statusInterval: null
};

let calibrationValue = 0;

/*
 * ============================================================================
 * 🔎 DOM HELPERS
 * ============================================================================
 */

function $(id) {
    return document.getElementById(id);
}

function updateStatusText(message) {
    const el = $("status");

    if (el) {
        el.innerText = "Status: " + message;
    }
}

function updateMessage(message) {
    const el = $("message");

    if (el) {
        el.innerText = message;
    }
}

/*
 * ============================================================================
 * 🎛️ BOTÕES
 * ============================================================================
 */

function setActionButtonsDisabled(disabled) {
    const btnInc = $("btnInc");
    const btnDec = $("btnDec");

    if (btnInc) {
        btnInc.disabled = disabled;
    }

    if (btnDec) {
        btnDec.disabled = disabled;
    }
}

function setStopButtonDisabled(disabled) {
    const btnStop = $("btnStop");

    if (btnStop) {
        btnStop.disabled = disabled;
    }
}

/*
 * ============================================================================
 * 🔐 LOGIN / LOGOUT
 * ============================================================================
 */

async function login() {
    const pinInput = $("pin");

    if (!pinInput || !pinInput.value) {
        alert("Digite o PIN");
        return;
    }

    try {
        updateStatusText("Autenticando...");

        const response = await fetch("/api/login", {
            method: "POST",
            body: pinInput.value
        });

        if (!response.ok) {
            updateStatusText("PIN incorreto");
            alert("PIN incorreto");
            return;
        }

        AppState.token = await response.text();

        /*
         * Mantém sessão por reload/troca de página.
         */
        localStorage.setItem("seringa_token", AppState.token);

        const loginBox = $("loginBox");

        if (loginBox) {
            loginBox.style.display = "none";
        }

        updateMessage("");
        updateStatusText("Autenticado");

        setActionButtonsDisabled(false);
        setStopButtonDisabled(true);

        await updateStatus();
        await calibrationLoad();

        startStatusPolling();

    } catch (error) {
        updateStatusText("Erro de conexão");
    }
}

function logout() {
    AppState.token = null;

    localStorage.removeItem("seringa_token");

    if (AppState.statusInterval) {
        clearInterval(AppState.statusInterval);
        AppState.statusInterval = null;
    }

    const loginBox = $("loginBox");

    if (loginBox) {
        loginBox.style.display = "block";
    }

    setActionButtonsDisabled(true);
    setStopButtonDisabled(true);

    updateMessage("");
    updateStatusText("Sessão expirada");
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
 * 📡 API
 * ============================================================================
 */

async function apiRequest(endpoint, options = {}) {
    if (!AppState.token) {
        alert("Faça login primeiro");
        return null;
    }

    try {
        const headers = options.headers || {};

        headers["Authorization"] = AppState.token;

        const response = await fetch(endpoint, {
            ...options,
            headers: headers
        });

        if (response.status === 401) {
            logout();
            return null;
        }

        if (!response.ok) {
            updateStatusText("Erro HTTP: " + response.status);
            return null;
        }

        return response;

    } catch (error) {
        updateStatusText("Falha de conexão");
        return null;
    }
}

/*
 * ============================================================================
 * 📊 STATUS
 * ============================================================================
 */

async function updateStatus() {
    if (!AppState.token) {
        return;
    }

    const response = await apiRequest("/api/status", {
        method: "GET"
    });

    if (!response) {
        return;
    }

    try {
        const data = await response.json();

        const isBusy =
            data.busy === true ||
            data.busy === "true" ||
            data.motor === "RUNNING" ||
            data.seringa === "MOVING";

        updateStatusText(
            "Seringa: " + data.seringa +
            " | Motor: " + data.motor +
            " | Busy: " + (isBusy ? "sim" : "não")
        );

        setActionButtonsDisabled(isBusy);
        setStopButtonDisabled(!isBusy);

    } catch (error) {
        updateStatusText("Erro ao interpretar status");
    }
}

function startStatusPolling() {
    if (AppState.statusInterval) {
        return;
    }

    AppState.statusInterval = setInterval(
        updateStatus,
        500
    );
}

/*
 * ============================================================================
 * ⚙️ COMANDOS DA SERINGA
 * ============================================================================
 */

async function sendMotorCommand(endpoint) {
    setActionButtonsDisabled(true);
    setStopButtonDisabled(false);

    updateStatusText("Enviando comando...");

    const response = await apiRequest(endpoint, {
        method: "GET"
    });

    if (!response) {
        await updateStatus();
        return;
    }

    const text = await response.text();

    if (text === "BUSY") {
        updateStatusText("Motor ocupado");
        await updateStatus();
        return;
    }

    updateStatusText("Comando enviado");
    await updateStatus();
}

function inc() {
    sendMotorCommand("/api/inc");
}

function dec() {
    sendMotorCommand("/api/dec");
}

async function stopMotor() {
    setStopButtonDisabled(true);

    updateStatusText("Enviando STOP...");

    const response = await apiRequest("/api/stop", {
        method: "GET"
    });

    if (response) {
        updateStatusText("STOP enviado");
    }

    await updateStatus();
}

/*
 * ============================================================================
 * 🧪 CALIBRAGEM
 * ============================================================================
 *
 * Valor: steps/ml
 *
 * A tela altera o valor localmente e só persiste quando clicar em SALVAR.
 * ============================================================================
 */

function updateCalibrationValue() {
    const el = $("value");

    if (el) {
        el.innerText = calibrationValue.toFixed(0);
    }
}

async function calibrationLoad() {
    /*
     * Se a página não tem elemento "value",
     * não estamos na tela de calibragem.
     */
    if (!$("value")) {
        return;
    }

    const response = await apiRequest("/api/calibration/get", {
        method: "GET"
    });

    if (!response) {
        return;
    }

    try {
        const data = await response.json();

        calibrationValue = Number(data.steps_per_ml);

        updateCalibrationValue();
        updateStatusText("Calibração carregada");

    } catch (error) {
        updateStatusText("Erro lendo calibração");
    }
}

async function calSave() {
    if (!$("value")) {
        return;
    }

    updateStatusText("Salvando calibração...");

    const response = await apiRequest("/api/calibration/set", {
        method: "POST",
        body: calibrationValue.toString()
    });

    if (!response) {
        return;
    }

    updateStatusText("Calibração salva");
}

function calInc() {
    calibrationValue += 50;
    updateCalibrationValue();
}

function calDec() {
    calibrationValue -= 50;

    if (calibrationValue < 100) {
        calibrationValue = 100;
    }

    updateCalibrationValue();
}

/*
 * ============================================================================
 * 🚀 BOOT
 * ============================================================================
 */

async function boot() {
    updateCalibrationValue();

    if (AppState.token) {
        const loginBox = $("loginBox");

        if (loginBox) {
            loginBox.style.display = "none";
        }

        setActionButtonsDisabled(false);
        setStopButtonDisabled(true);

        await updateStatus();
        await calibrationLoad();

        startStatusPolling();

        return;
    }

    setActionButtonsDisabled(true);
    setStopButtonDisabled(true);

    if ($("value")) {
        updateStatusText("faça login na tela inicial");
    } else {
        updateStatusText("aguardando login");
    }
}

boot();