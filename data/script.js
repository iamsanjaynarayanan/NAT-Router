// Function to switch to Scan View and trigger the ESP32 to scan
function showScan() {
    document.getElementById('main-ui').style.display = 'none';
    document.getElementById('scan-ui').style.display = 'block';
    
    const resultsTable = document.getElementById('scan-results');
    resultsTable.innerHTML = '<tr><td colspan="3" style="text-align:center;">Scanning Airwaves...</td></tr>';

    fetch('/scan')
        .then(response => response.json())
        .then(data => {
            resultsTable.innerHTML = '';
            if (data.length === 0) {
                resultsTable.innerHTML = '<tr><td colspan="3">No networks found.</td></tr>';
                return;
            }
            data.forEach(net => {
                let row = `<tr>
                    <td><strong>${net.ssid}</strong></td>
                    <td>${net.rssi} dBm</td>
                    <td><button class="btn-primary" style="padding: 5px 10px; width: auto;" 
                        onclick="selectNetwork('${net.ssid}')">Select</button></td>
                </tr>`;
                resultsTable.innerHTML += row;
            });
        })
        .catch(err => {
            resultsTable.innerHTML = '<tr><td colspan="3">Scan failed. Try again.</td></tr>';
        });
}

// THIS IS THE KEY: It takes the scanned SSID and puts it in the input box
function selectNetwork(ssid) {
    document.getElementById('sta_ssid').value = ssid;
    showHome();
}

function showHome() {
    document.getElementById('main-ui').style.display = 'block';
    document.getElementById('scan-ui').style.display = 'none';
}

// Sends the data to the ESP32 backend
function saveConfig() {
    const ap_ssid = document.getElementById('ap_ssid').value;
    const ap_pass = document.getElementById('ap_pass').value;
    const sta_ssid = document.getElementById('sta_ssid').value;
    const sta_pass = document.getElementById('sta_pass').value;

    if (!sta_ssid) {
        alert("Please enter or select an Uplink SSID");
        return;
    }

    let params = new URLSearchParams();
    params.append('ap_ssid', ap_ssid);
    params.append('ap_pass', ap_pass);
    params.append('sta_ssid', sta_ssid);
    params.append('sta_pass', sta_pass);

    fetch('/save', { method: 'POST', body: params })
        .then(response => {
            if(response.ok) {
                alert("Configuration Saved! The ESP32 will now reboot and attempt to connect.");
            }
        });
}