// Hardcoded dashboard authorization check
const AUTH_CONFIG = { user: "ramadan", pass: "12345678" };

function handleLogin(event) {
    event.preventDefault();
    const user = document.getElementById("username").value;
    const pass = document.getElementById("password").value;
    const errorDiv = document.getElementById("auth-error");

    if (user === AUTH_CONFIG.user && pass === AUTH_CONFIG.pass) {
        document.getElementById("auth-screen").classList.add("dashboard-hidden");
        document.getElementById("main-dashboard").classList.remove("dashboard-hidden");
    } else {
        errorDiv.style.display = "block";
    }
}

// 1. Unified Live Hardware Reset Request via Fetch API
function resetEnergy() {
    if (confirm("Are you sure you want to reset energy metrics?")) {
        console.log("Sending Reset Request to ESP32 Endpoint...");
        
        fetch('/api/reset', { method: 'POST' })
            .then(response => {
                if (!response.ok) throw new Error("Could not clear hardware registers.");
                alert("Energy metrics usage successfully reset.");
            })
            .catch(error => {
                console.error("Reset action aborted:", error);
                alert("Error resetting hardware energy profile.");
            });
    }
}

// 2. Relay Power ON/OFF Toggle Handler
let isPowerOn = true;
function togglePower() {
    const btn = document.getElementById("power-btn");
    const targetState = !isPowerOn; 
    const stateQueryValue = targetState ? 1 : 0;

    console.log(`ESP32 Command: Set Relay state to ${stateQueryValue}`);

    fetch(`/api/power?state=${stateQueryValue}`, { method: 'POST' })
        .then(response => {
            if (!response.ok) {
                throw new Error("Hardware failed to switch power interlock relay.");
            }
            return response.text();
        })
        .then(() => {
            isPowerOn = targetState;

            if (isPowerOn) {
                btn.className = "btn-tactile power-on";
                btn.innerText = "Power: ON";
            } else {
                btn.className = "btn-tactile power-off";
                btn.innerText = "Power: OFF";
            }
        })
        .catch(error => {
            console.error('Relay operation control fault:', error);
            alert("Failed to process command. Power state unchanged or credit depleted.");
        });
}

// 3. Recharge Modal Keypad Controller Functions
function openRechargeModal() {
    const inputDisplay = document.getElementById("recharge-amount");
    inputDisplay.value = "0";
    document.getElementById("recharge-modal").classList.remove("modal-hidden");
}

function closeRechargeModal() {
    document.getElementById("recharge-modal").classList.add("modal-hidden");
}

function pressKey(num) {
    const inputDisplay = document.getElementById("recharge-amount");
    if (num === 'C') {
        inputDisplay.value = "0";
        return; 
    }
    
    if (inputDisplay.value === "0") {
        inputDisplay.value = num;
    } else {
        if (inputDisplay.value.length < 6) {
            inputDisplay.value += num;
        }
    }
}

function submitRecharge() {
    const inputDisplay = document.getElementById("recharge-amount");
    const amount = parseInt(inputDisplay.value, 10);
    if (amount <= 0 || isNaN(amount)) {
        alert("Please enter a valid recharge amount.");
        return;
    }

    console.log(`Sending top-up payload to ESP32: ${amount} Naira`);
    
    fetch(`/api/recharge?amount=${amount}`, { method: 'POST' })
        .then(response => {
            if (!response.ok) {
                throw new Error("Server rejected top-up profile validation request.");
            }
            return response.text();
        })
        .then(() => {
            const creditElement = document.getElementById("val-credit");
            let currentVal = parseFloat(creditElement.innerText.replace('₦', ''));
            creditElement.innerText = "₦" + (currentVal + amount).toFixed(2);

            closeRechargeModal();
            alert(`Successfully recharged ₦${amount}!`);
        })
        .catch(error => {
            console.error('Recharge transmission fault:', error);
            alert("Failed to submit recharge to energy meter. Check link connection.");
        });
}

// 4. Dynamic Station (STA) Wi-Fi Provisioning Handler
function submitWifiCredentials(event) {
    event.preventDefault();
    
    const ssid = document.getElementById("wifi-ssid").value;
    const password = document.getElementById("wifi-pass").value;
    const msgDiv = document.getElementById("wifi-status-message");

    msgDiv.style.display = "block";
    msgDiv.style.color = "#f59e0b"; // Warning orange color
    msgDiv.innerText = "Sending credentials... ESP32 will attempt connection.";

    const payload = {
        ssid: ssid,
        password: password
    };

    console.log("Provisioning target STA interface via JSON payload...");

    fetch('/api/wifi/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
    })
    .then(response => {
        if (!response.ok) {
            throw new Error("ESP32 HTTP server rejected provision profiles.");
        }
        return response.text();
    })
    .then(msg => {
        msgDiv.style.color = "#34d399"; // Success green color
        msgDiv.innerText = "Configuration accepted! Reconnecting station interface...";
        alert("Wi-Fi credentials saved. ESP32 is initiating station handshake connection.");
    })
    .catch(error => {
        console.error('Wi-Fi link update execution error:', error);
        msgDiv.style.color = "#ef4444"; // Error red color
        msgDiv.innerText = "Failed to dispatch network structural settings.";
        alert("Error updating Wi-Fi configurations. Verify physical interface link.");
    });
}

document.addEventListener('DOMContentLoaded', () => {
    setInterval(fetchPfcMetrics, 2000);
});

function fetchPfcMetrics() {
    fetch('/data')
        .then(response => {
            if (!response.ok) throw new Error("Network issues reading meter.");
            return response.json();
        })
        .then(data => {
            document.getElementById('val-volt').innerText = data.voltage.toFixed(1) + " V";
            document.getElementById('val-curr').innerText = data.current.toFixed(3) + " A";
            document.getElementById('val-ap').innerText   = data.power.toFixed(1) + " W";
            document.getElementById('val-energy').innerText = data.energy.toFixed(3) + " kWh";
            document.getElementById('val-freq').innerText   = data.frequency.toFixed(1) + " Hz";
            document.getElementById('val-pf').innerText     = data.pf.toFixed(2);
            document.getElementById('val-credit').innerText = "₦" + data.credit.toFixed(2);
            
            const pfBadge = document.getElementById('val-pf');
            if (data.pf < 0.85) {
                pfBadge.style.color = "#ff3b30";
            } else {
                pfBadge.style.color = "#a7f3d0";
            }

            const btn = document.getElementById("power-btn");
            if (data.credit <= 3.0) {
                isPowerOn = false; 
                btn.className = "btn-tactile power-off";
                btn.innerText = "Power: OFF (No Credit)";
                btn.style.opacity = "0.6";
            } else {
                btn.style.opacity = "1.0";
            }

            const currentMask = data.cap_banks;
            for (let i = 0; i < 4; i++) {
                const badge = document.getElementById(`cap-ind-${i}`);
                if (badge) {
                    if (currentMask & (1 << i)) {
                        badge.classList.add("indicator-active");
                    } else {
                        badge.classList.remove("indicator-active");
                    }
                }
            }
        })
        .catch(error => console.error('Data pipeline error:', error));
}
