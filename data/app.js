// Copyright 2026 clesiorki
//
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root for full license information.

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
    authenticated: sessionStorage.getItem("seringa_authenticated") === "1",
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

function formatGpioLevel(level) {
    return Number(level) === 1 ? "HIGH" : "LOW";
}

function updateEndstopDiagnostics(data) {
    const el = $("endstops");

    if (!el || !data) {
        return;
    }

    const vazioLevel =
        formatGpioLevel(data.endstop_vazio_level);

    const cheioLevel =
        formatGpioLevel(data.endstop_cheio_level);

    el.innerText =
        "Endstops: vazio GPIO" +
        data.endstop_vazio_gpio +
        "=" +
        vazioLevel +
        " " +
        (data.vazia ? "ACIONADO" : "livre") +
        " | cheio GPIO" +
        data.endstop_cheio_gpio +
        "=" +
        cheioLevel +
        " " +
        (data.cheia ? "ACIONADO" : "livre");
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
        AppState.authenticated = true;

        /*
         * Mantém sessão por reload/troca de página.
         */
        localStorage.setItem("seringa_token", AppState.token);
        sessionStorage.setItem("seringa_authenticated", "1");

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
    AppState.authenticated = false;

    localStorage.removeItem("seringa_token");
    sessionStorage.removeItem("seringa_authenticated");

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
    if (!AppState.token || !AppState.authenticated) {
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
    /*
     * ================================================================
     * 🔐 POLLING APENAS COM LOGIN ATIVO
     * ================================================================
     *
     * Token salvo em localStorage não significa sessão válida no ESP32:
     * após reboot do firmware, o backend perde o token em RAM.
     *
     * Por isso status só é consultado depois de login bem-sucedido nesta
     * execução da página. Isso evita spam de:
     *
     *  httpd_resp_send_err: 401 Unauthorized - Sessão inválida
     */
    if (!AppState.token || !AppState.authenticated) {
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

        updateEndstopDiagnostics(data);

        setActionButtonsDisabled(isBusy);
        setStopButtonDisabled(!isBusy);

    } catch (error) {
        updateStatusText("Erro ao interpretar status");
    }
}

function startStatusPolling() {
    if (!AppState.authenticated) {
        return;
    }

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
    calibrationValue += 1000;
    updateCalibrationValue();
}

function calDec() {
    calibrationValue -= 1000;

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

    if (AppState.token && AppState.authenticated) {
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
