// preferences.js
/**
 * Global Application State Management
 */
const AppState = {
    refreshInterval: null,
};
/**
 * Logging Utility
 */
const Logger = {
    log(message, data = null) {
        console.log(`[Preferences] ${message}`, data || '');
    },
    
    error(message, error = null) {
        console.error(`[Preferences Error] ${message}`, error || '');
    },
    
    warn(message) {
        console.warn(`[Preferences Warning] ${message}`);
    }
};
/**
 * Authentication and Network Utilities
 */
const AuthUtils = {
    async checkAuth() {
        try {
            Logger.log('Attempting authentication check...');
            
            const response = await fetch('/api/sensors', {
                credentials: 'include',
                method: 'GET'
            });
            
            switch (response.status) {
                case 200:
                    Logger.log('Authentication successful');
                    return true;
                case 401:
                    Logger.warn('Unauthorized access');
                    this.redirectToLogin();
                    return false;
                case 403:
                    Logger.error('Access forbidden');
                    showStatus('Access denied. Contact administrator.', true);
                    return false;
                default:
                    Logger.error(`Unexpected authentication response: ${response.status}`);
                    return false;
            }
        } catch (error) {
            Logger.error('Authentication Check Failed', error);
            this.redirectToLogin();
            return false;
        }
    },

    redirectToLogin() {
        Logger.warn('Redirecting to login page');
        window.location.href = '/login';
    },

    async logout() {
        try {
            const response = await fetch('/api/logout', {
                method: 'POST',
                credentials: 'include'
            });
            
            if (response.ok) {
                this.redirectToLogin();
            } else {
                throw new Error('Logout failed');
            }
        } catch (error) {
            showStatus('Logout failed. Please try again.', true);
        }
    }
};
/**
 * UI Utilities
 */
function togglePasswordVisibility(inputId) {
    const input = document.getElementById(inputId);
    const icon = input.nextElementSibling.querySelector('i');
    if (input.type === 'password') {
        input.type = 'text';
        icon.setAttribute('data-lucide', 'eye-off');
    } else {
        input.type = 'password';
        icon.setAttribute('data-lucide', 'eye');
    }
    lucide.createIcons();
}
function showStatus(message, isError = false) {
    const statusDiv = document.getElementById('statusMessage');
    statusDiv.textContent = message;
    statusDiv.className = `mb-4 p-4 rounded-md ${
        isError 
            ? 'bg-red-100 text-red-700 border border-red-400' 
            : 'bg-green-100 text-green-700 border border-green-400'
    }`;
    statusDiv.classList.remove('hidden');
    
    setTimeout(() => {
        statusDiv.classList.add('hidden');
    }, 5000);
}
/**
 * Credentials Management
 */
async function loadCurrentUsername() {
    try {
        const response = await fetch('/api/credentials', {
            credentials: 'include'
        });
        
        if (response.ok) {
            const data = await response.json();
            document.getElementById('current-username').textContent = data.username;
        }
    } catch (error) {
        Logger.error('Error loading username:', error);
        document.getElementById('current-username').textContent = 'Error loading username';
    }
}
async function handleCredentialsSubmit(event) {
    event.preventDefault();
    
    const currentPassword = document.getElementById('current-password').value;
    const newUsername = document.getElementById('new-username').value.trim();
    const newPassword = document.getElementById('new-password').value;
    const confirmPassword = document.getElementById('confirm-password').value;
    
    // Validation
    if (newPassword && newPassword !== confirmPassword) {
        showStatus('New passwords do not match', true);
        return;
    }
    
    if (!currentPassword) {
        showStatus('Current password is required', true);
        return;
    }
    
    if (newUsername && (newUsername.length < 3 || newUsername.length > 32)) {
        showStatus('Username must be between 3 and 32 characters', true);
        return;
    }
    
    if (newPassword && (newPassword.length < 8 || newPassword.length > 64)) {
        showStatus('Password must be between 8 and 64 characters', true);
        return;
    }
    
    try {
        const response = await fetch('/api/credentials', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            credentials: 'include',
            body: JSON.stringify({
                current_password: currentPassword,
                new_username: newUsername || undefined,
                new_password: newPassword || undefined
            })
        });
        
        if (response.ok) {
            showStatus('Credentials updated successfully');
            document.getElementById('credentialsForm').reset();
            loadCurrentUsername();
        } else {
            const error = await response.json();
            showStatus(error.error || 'Failed to update credentials', true);
        }
    } catch (error) {
        Logger.error('Error updating credentials:', error);
        showStatus('Failed to update credentials', true);
    }
}
/**
 * Preferences Management
 */
async function loadData() {
    try {
        const [prefsResponse, sensorsResponse] = await Promise.all([
            fetch('/api/preferences', { credentials: 'include' }),
            fetch('/api/sensors', { credentials: 'include' })
        ]);

        if (!prefsResponse.ok || !sensorsResponse.ok) {
            throw new Error('Failed to load data from server');
        }

        const [preferences, sensors] = await Promise.all([
            prefsResponse.json(),
            sensorsResponse.json()
        ]);

        Logger.log('Loaded preferences:', preferences);
        Logger.log('Loaded sensors:', sensors);

        // Update MQTT settings
        if (preferences.mqtt) {
            document.getElementById('mqtt.broker').value = preferences.mqtt.broker || '';
            document.getElementById('mqtt.port').value = preferences.mqtt.port || '';
            document.getElementById('mqtt.username').value = preferences.mqtt.username || '';
        }

        // Update scanning settings
        if (preferences.scanning) {
            document.getElementById('scanning.autoScanEnabled').checked = 
                Boolean(preferences.scanning.autoScanEnabled);
            document.getElementById('scanning.scanInterval').value = 
                preferences.scanning.scanInterval || 30;
        }

        // Update relay names
        if (preferences.relays) {
            preferences.relays.forEach(relay => {
                const input = document.getElementById(`relay-${relay.relay_id}-name`);
                if (input) {
                    input.value = relay.name || '';
                }
            });
        }

        // Update sensor list and display sensor selection
        if (!preferences.sensors) {
            preferences.sensors = {};
        }
        updateSensorList(sensors, preferences);

    } catch (error) {
        Logger.error('Error loading data:', error);
        showStatus('Failed to load data: ' + error.message, true);
    }
}
/**
 * Sensor Management
 */
function updateSensorList(sensors, preferences) {
    const sensorList = document.getElementById('sensorList');
    const displaySelect = document.getElementById('display.selectedSensor');
    
    if (!sensorList || !displaySelect) return;

    // Clear existing content
    sensorList.innerHTML = '';
    while (displaySelect.options.length > 1) {
        displaySelect.remove(1);
    }

    // Update both lists
    sensors.forEach(sensor => {
        // Add to display sensor dropdown
        const option = document.createElement('option');
        option.value = sensor.address;
        const name = preferences.sensors[sensor.address] || `Sensor ${sensor.address.slice(-4)}`;
        option.textContent = sensor.valid 
            ? `${name} (${sensor.temperature.toFixed(1)}°C)` 
            : `${name} (Invalid)`;
        displaySelect.appendChild(option);

        // Create sensor name input
        const sensorDiv = document.createElement('div');
        sensorDiv.className = 'flex items-start space-x-4 p-4 bg-gray-50 rounded-lg';
        sensorDiv.innerHTML = `
            <div class="flex-1 min-w-0">
                <div class="flex items-center space-x-2">
                    <input type="text"
                           name="sensor-${sensor.address}"
                           value="${preferences.sensors[sensor.address] || ''}"
                           placeholder="Sensor ${sensor.address.slice(-4)}"
                           class="flex-1 rounded-md border-gray-300 shadow-sm focus:border-blue-500 focus:ring-blue-500">
                </div>
                <div class="mt-1 text-sm text-gray-500">
                    ID: ${sensor.address}
                    ${sensor.valid ? 
                        ` • Current: ${sensor.temperature.toFixed(1)}°C` : 
                        ' • Status: Invalid'}
                </div>
            </div>
        `;
        sensorList.appendChild(sensorDiv);
    });

    // Set selected display sensor if present
    if (preferences.display?.selectedSensor) {
        displaySelect.value = preferences.display.selectedSensor;
    }
}
/**
 * Form Data Management
 */
function collectFormData() {
    const formData = {
        mqtt: {
            broker: document.getElementById('mqtt.broker').value,
            port: parseInt(document.getElementById('mqtt.port').value),
            username: document.getElementById('mqtt.username').value || '',
            password: document.getElementById('mqtt.password').value || ''
        },
        scanning: {
            autoScanEnabled: document.getElementById('scanning.autoScanEnabled').checked,
            scanInterval: parseInt(document.getElementById('scanning.scanInterval').value)
        },
        display: {
            selectedSensor: document.getElementById('display.selectedSensor').value,
            brightnessLevel: 7,
            displayTimeout: 30
        },
        sensors: {},
        relays: []
    };

    // Clean up empty MQTT fields
    if (!formData.mqtt.username) delete formData.mqtt.username;
    if (!formData.mqtt.password) delete formData.mqtt.password;

    // Collect sensor names
    const sensorInputs = document.querySelectorAll('input[name^="sensor-"]');
    sensorInputs.forEach(input => {
        const name = input.value.trim();
        if (name) {
            const address = input.name.replace('sensor-', '');
            formData.sensors[address] = name;
        }
    });

    // Collect relay names
    for (let i = 0; i < 2; i++) {
        const name = document.getElementById(`relay-${i}-name`).value.trim();
        if (name) {
            formData.relays.push({
                relay_id: i,
                name: name
            });
        }
    }

    return formData;
}
async function handlePreferencesSubmit(event) {
    event.preventDefault();
    
    try {
        const formData = collectFormData();
        Logger.log('Sending preferences update:', formData);

        const response = await fetch('/api/preferences', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            credentials: 'include',
            body: JSON.stringify(formData)
        });

        if (!response.ok) {
            let errorMessage = 'Failed to save preferences';
            try {
                const errorData = await response.json();
                errorMessage = errorData.error || errorMessage;
            } catch (e) {
                Logger.error('Error parsing error response:', e);
            }
            throw new Error(errorMessage);
        }

        showStatus('Preferences saved successfully');
        document.getElementById('mqtt.password').value = '';
        
        // Reload data to show updated values
        await loadData();

    } catch (error) {
        Logger.error('Error saving preferences:', error);
        showStatus(error.message, true);
    }
}
/**
 * Page Initialization
 */
async function initializePreferences() {
    Logger.log('Initializing Preferences...');

    try {
        // Authentication check
        const isAuthenticated = await AuthUtils.checkAuth();
        if (!isAuthenticated) {
            Logger.error('Authentication failed');
            return;
        }

        // Load initial data
        await loadData();
        await loadCurrentUsername();

        // Set up event listeners
        const preferencesForm = document.getElementById('preferencesForm');
        const credentialsForm = document.getElementById('credentialsForm');
        const refreshButton = document.getElementById('refreshButton');
        const logoutButton = document.getElementById('logoutButton');

        if (preferencesForm) {
            preferencesForm.addEventListener('submit', handlePreferencesSubmit);
        }

        if (credentialsForm) {
            credentialsForm.addEventListener('submit', handleCredentialsSubmit);
        }

        if (refreshButton) {
            refreshButton.addEventListener('click', loadData);
        }

        if (logoutButton) {
            logoutButton.addEventListener('click', () => AuthUtils.logout());
        }

        // Set up periodic refresh
        if (AppState.refreshInterval) {
            clearInterval(AppState.refreshInterval);
        }
        AppState.refreshInterval = setInterval(loadData, 30000);

        // Initialize Lucide icons
        if (typeof lucide !== 'undefined' && lucide.createIcons) {
            lucide.createIcons();
            Logger.log('Lucide icons initialized');
        } else {
            Logger.warn('Lucide icons not available');
        }

    } catch (error) {
        Logger.error('Preferences Initialization Failed', error);
        showStatus('Failed to initialize preferences', true);
    }
}
// Initialize on DOM Load
document.addEventListener('DOMContentLoaded', initializePreferences);