// Hardcoded dashboard authorization check
const AUTH_CONFIG = { user: "admin", pass: "1234" };

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
  // Calculate intended state instead of flipping flag prematurely
  const targetState = !isPowerOn; 
  const stateQueryValue = targetState ? 1 : 0;

  console.log(`ESP32 Command: Set Relay state to ${stateQueryValue}`);

  // Fire live state modification route to the webserver backend endpoints
  fetch(`/api/power?state=${stateQueryValue}`, { method: 'POST' })
    .then(response => {
      if (!response.ok) {
        throw new Error("Hardware failed to switch power interlock relay.");
      }
      return response.text();
    })
    .then(() => {
      // Commit state change only after network confirmation
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
const inputDisplay = document.getElementById("recharge-amount");

function openRechargeModal() {
  inputDisplay.value = "0";
  document.getElementById("recharge-modal").classList.remove("modal-hidden");
}

function closeRechargeModal() {
  document.getElementById("recharge-modal").classList.add("modal-hidden");
}

function pressKey(num) {
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
  const amount = parseInt(inputDisplay.value, 10);
  if (amount <= 0 || isNaN(amount)) {
    alert("Please enter a valid recharge amount.");
    return;
  }

  console.log(`Sending top-up payload to ESP32: ${amount} Naira`);
  
  // Fire network payload matching your query string routing structure
  fetch(`/api/recharge?amount=${amount}`, { method: 'POST' })
    .then(response => {
      if (!response.ok) {
        throw new Error("Server rejected top-up profile validation request.");
      }
      return response.text();
    })
    .then(() => {
      // ONLY alter local visual tracking nodes if hardware reports confirmation success
      const creditElement = document.getElementById("credit");
      let currentVal = parseFloat(creditElement.innerText.replace('₦', '')); // Strip symbol if present
      creditElement.innerText = "₦" + (currentVal + amount).toFixed(2);

      closeRechargeModal();
      alert(`Successfully recharged ₦${amount}!`);
    })
    .catch(error => {
      console.error('Recharge transmission fault:', error);
      alert("Failed to submit recharge to energy meter. Check link connection.");
    });
}


document.addEventListener('DOMContentLoaded', () => {
    // Check values every 2 seconds matching the dashboard subtitle layout
    setInterval(fetchPfcMetrics, 2000);
});

// 4. Enhanced JSON Parser loop matching full PZEM schema output
// Enhanced JSON Parser loop matching full PZEM schema and system state flags
function fetchPfcMetrics() {
    fetch('/data')
        .then(response => {
            if (!response.ok) throw new Error("Network issues reading meter.");
            return response.json();
        })
        .then(data => {
            // 1. Target and update basic telemetry items
            document.getElementById('val-volt').innerText = data.voltage.toFixed(1) + " V";
            document.getElementById('val-curr').innerText = data.current.toFixed(3) + " A";
            document.getElementById('val-ap').innerText   = data.power.toFixed(1) + " W";
            document.getElementById('val-energy').innerText = data.energy.toFixed(3) + " kWh";
            document.getElementById('val-freq').innerText   = data.frequency.toFixed(1) + " Hz";
            document.getElementById('val-pf').innerText     = data.pf.toFixed(2);
            document.getElementById('val-credit').innerText = "₦" + data.credit.toFixed(2);
            
            // 2. Efficiency coloration styling matching low power factors
            const pfBadge = document.getElementById('pf');
            if (data.pf < 0.85) {
                pfBadge.style.color = "#ff3b30"; // Red Warning
            } else {
                pfBadge.style.color = "#a7f3d0"; // Normal Neon Mint Green
            }

            // 3. CRITICAL HARDWARE STATE OVERRIDE SYNCHRONIZATION
            // If the meter balance drops below your threshold (3.0 ₦), your ESP32 drops the load.
            // We read the dynamic current to see if the relay is actively offline.
            const btn = document.getElementById("power-btn");

            if (data.credit <= 3.0) {
                // Force sync the UI state variables to structural "OFF"
                isPowerOn = false; 
                btn.className = "btn-tactile power-off";
                btn.innerText = "Power: OFF (No Credit)";
                btn.style.opacity = "0.6"; // Dim button slightly to indicate lock
            } else {
                btn.style.opacity = "1.0"; // Restore standard interaction look
                
                // If a user recharges and resets the system, this handles bringing it back inline
                // checking if current is zero despite a user attempting an "ON" software layout state
                if (data.power === 0 && data.voltage > 100 && isPowerOn) {
                    // Safety check if the line state implies the relay tripped unexpectedly
                    console.log("Telemetry check: Line active but zero load consumption detected.");
                }
            }

            // 4. Update the 4 Unique Capacitor Relay State Badges via Bitmask Parsing
            const currentMask = data.cap_banks; // e.g. 5 means Bank 1 (bit 0) and Bank 3 (bit 2) are active

            for (let i = 0; i < 4; i++) {
                const badge = document.getElementById(`cap-ind-${i}`);
                if (badge) {
                    // Bitwise check if the i-th bit is set to high (1)
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

