// Temperature Monitoring Dashboard
// Comprehensive client-side functionality for sensor monitoring system
/**
 * Global Application State Management
 */
const AppState = {
    selectedSensor: null,     // Currently selected sensor address
    sensorHistory: {},        // Temperature history for all sensors
    chart: null,              // Chart.js instance for visualization
    refreshInterval: null,    // Interval for automatic data refresh
    relayStates: {           // Current state of relays
        0: false,
        1: false
    }
};
/**
 * Logging Utility
 */
const Logger = {
    log(message, data = null) {
        console.log(`[Dashboard] ${message}`, data || '');
    },
    
    error(message, error = null) {
        console.error(`[Dashboard Error] ${message}`, error || '');
    },
    
    warn(message) {
        console.warn(`[Dashboard Warning] ${message}`);
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
            
            Logger.log('Authentication Response', {
                status: response.status,
                ok: response.ok
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
                    this.showError('Access denied. Contact administrator.');
                    return false;
                default:
                    Logger.error(`Unexpected authentication response: ${response.status}`);
                    return false;
            }
        } catch (error) {
            Logger.error('Authentication Check Failed', {
                message: error.message,
                name: error.name,
                stack: error.stack
            });

            if (error instanceof TypeError) {
                this.showError('Network error. Check your connection.');
            }

            this.redirectToLogin();
            return false;
        }
    },

    redirectToLogin() {
        Logger.warn('Redirecting to login page');
        window.location.href = '/login';
    },

    showError(message) {
        Logger.error(message);
        const alertDiv = document.createElement('div');
        alertDiv.className = 'bg-red-100 border border-red-400 text-red-700 px-4 py-3 rounded relative';
        alertDiv.textContent = message;
        
        const alertContainer = document.getElementById('alertContainer');
        if (alertContainer) {
            alertContainer.appendChild(alertDiv);
            setTimeout(() => {
                if (alertDiv.parentNode) {
                    alertContainer.removeChild(alertDiv);
                }
            }, 5000);
        }
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
            this.showError('Logout failed. Please try again.');
        }
    }
};
/**
 * Sensor Selection Management
 */
const SensorSelectionManager = {
    STORAGE_KEY: 'selectedChartSensors',

    saveSelectedSensors(sensors) {
        try {
            localStorage.setItem(this.STORAGE_KEY, JSON.stringify(sensors));
            Logger.log('Saved selected sensors:', sensors);
        } catch (error) {
            Logger.error('Failed to save sensor selections', error);
        }
    },

    getSelectedSensors() {
        try {
            const savedSelections = localStorage.getItem(this.STORAGE_KEY);
            return savedSelections ? JSON.parse(savedSelections) : [];
        } catch (error) {
            Logger.error('Failed to retrieve saved sensor selections', error);
            return [];
        }
    },

    clearSelectedSensors() {
        try {
            localStorage.removeItem(this.STORAGE_KEY);
            Logger.log('Cleared saved sensor selections');
        } catch (error) {
            Logger.error('Failed to clear sensor selections', error);
        }
    }
};
/**
 * Chart Visualization Utilities
 */
const ChartUtils = {
    initChart() {
        const chartCanvas = document.getElementById('temperatureChart');
        
        if (!chartCanvas) {
            Logger.error('Temperature chart canvas not found!');
            return null;
        }

        const ctx = chartCanvas.getContext('2d');
        
        return new Chart(ctx, {
            type: 'line',
            data: {
                datasets: []
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    x: {
                        type: 'time',
                        time: {
                            unit: 'minute',
                            displayFormats: {
                                minute: 'HH:mm'
                            }
                        },
                        title: {
                            display: true,
                            text: 'Time'
                        }
                    },
                    y: {
                        title: {
                            display: true,
                            text: 'Temperature (°C)'
                        }
                    }
                },
                plugins: {
                    legend: {
                        display: true,
                        position: 'top'
                    }
                }
            }
        });
    },

    updateChart() {
        if (!AppState.chart) {
            Logger.error('Chart not initialized');
            return;
        }

        const selectedSensors = SensorSelectionManager.getSelectedSensors();
        AppState.chart.data.datasets = [];

        Object.entries(AppState.sensorHistory).forEach(([address, history]) => {
            if (selectedSensors.length > 0 && !selectedSensors.includes(address)) {
                return;
            }

            if (history.length === 0) return;

            const sensorName = this.getSensorName(address);

            AppState.chart.data.datasets.push({
                label: sensorName,
                data: history.map(entry => ({
                    x: entry.time,
                    y: entry.temp
                })),
                borderColor: this.getRandomColor(),
                tension: 0.1
            });
        });

        AppState.chart.update();
    },

    getRandomColor() {
        const letters = '0123456789ABCDEF';
        let color = '#';
        for (let i = 0; i < 6; i++) {
            color += letters[Math.floor(Math.random() * 16)];
        }
        return color;
    },

    getSensorName(address) {
        const sensorSelect = document.getElementById('sensorSelect');
        if (!sensorSelect) {
            Logger.error('Sensor select dropdown not found');
            return `Sensor ${address.slice(-4)}`;
        }
        
        const option = sensorSelect.querySelector(`option[value="${address}"]`);
        return option ? option.textContent : `Sensor ${address.slice(-4)}`;
    }
};
/**
 * Relay Control Management
 */
const RelayManager = {
    async initRelayControls() {
        const relay1Toggle = document.getElementById('relay1-toggle');
        const relay2Toggle = document.getElementById('relay2-toggle');

        if (relay1Toggle) {
            relay1Toggle.addEventListener('change', async (e) => {
                e.preventDefault();
                const success = await this.toggleRelay(0, e.target.checked);
                if (!success) {
                    e.target.checked = !e.target.checked;
                }
            });
        }

        if (relay2Toggle) {
            relay2Toggle.addEventListener('change', async (e) => {
                e.preventDefault();
                const success = await this.toggleRelay(1, e.target.checked);
                if (!success) {
                    e.target.checked = !e.target.checked;
                }
            });
        }

        await this.fetchRelayStates();
        setInterval(() => this.fetchRelayStates(), 5000);
    },

    async toggleRelay(relayId, state) {
        try {
            Logger.log(`Toggling Relay ${relayId} to ${state ? 'ON' : 'OFF'}`);
            
            const response = await fetch('/api/relay', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                credentials: 'include',
                body: JSON.stringify({
                    relay_id: relayId,
                    state: state
                })
            });

            if (!response.ok) {
                throw new Error(`Failed to toggle relay ${relayId}`);
            }

            AppState.relayStates[relayId] = state;
            Logger.log(`Relay ${relayId} state updated successfully`);
            return true;

        } catch (error) {
            Logger.error(`Relay Toggle Error: ${error.message}`);
            AuthUtils.showError(`Failed to toggle relay ${relayId + 1}`);
            return false;
        }
    },

    async fetchRelayStates() {
        try {
            const response = await fetch('/api/relay', {
                credentials: 'include'
            });

            if (!response.ok) {
                throw new Error('Failed to fetch relay states');
            }

            const relayStates = await response.json();
            
            relayStates.forEach(relay => {
                const relayId = relay.relay_id;
                const toggle = document.getElementById(`relay${relayId + 1}-toggle`);
                const label = document.getElementById(`relay${relayId + 1}-label`);
                
                if (toggle) {
                    toggle.checked = relay.state;
                    AppState.relayStates[relayId] = relay.state;
                }
                
                if (label && relay.name) {
                    label.textContent = relay.name;
                } else if (label) {
                    label.textContent = `Relay ${relayId + 1}`;
                }
            });

            Logger.log('Relay states updated from server');
        } catch (error) {
            Logger.error('Fetch Relay States Error:', error);
        }
    }
};
/**
 * Sensor Data Management
 */
const SensorManager = {
    async fetchSensors() {
        try {
            Logger.log('Fetching sensors...');
            
            const response = await fetch('/api/sensors', {
                credentials: 'include'
            });

            Logger.log('Sensor Fetch Response', {
                status: response.status,
                statusText: response.statusText
            });

            if (!response.ok) {
                throw new Error(`Sensor fetch failed: ${response.status}`);
            }

            const sensors = await response.json();
            Logger.log('Received Sensors', sensors);

            if (!Array.isArray(sensors)) {
                throw new Error('Invalid sensor data format');
            }

            this.processSensors(sensors);
        } catch (error) {
            Logger.error('Sensor Fetch Error', {
                message: error.message,
                name: error.name,
                stack: error.stack
            });
            
            AuthUtils.showError('Unable to retrieve sensor data');
        }
    },

    processSensors(sensors) {
        try {
            this.updateSensorSelect(sensors);
            this.updateSensorList(sensors);
            this.updateCurrentSensor(sensors);
            this.updateSensorHistory(sensors);

            if (!AppState.chart) {
                AppState.chart = ChartUtils.initChart();
            }
            
            ChartUtils.updateChart();
        } catch (error) {
            Logger.error('Sensor Processing Error', error);
        }
    },

    updateSensorSelect(sensors) {
        const sensorSelect = document.getElementById('sensorSelect');
        if (!sensorSelect) return;

        while (sensorSelect.options.length > 1) {
            sensorSelect.remove(1);
        }

        sensors.forEach(sensor => {
            const option = document.createElement('option');
            option.value = sensor.address;
            option.textContent = sensor.name || `Sensor ${sensor.address.slice(-4)}`;
            
            if (sensor.valid) {
                option.textContent += ` (${sensor.temperature.toFixed(1)}°C)`;
            }

            sensorSelect.appendChild(option);
        });
    },

    updateSensorList(sensors) {
        const sensorList = document.getElementById('sensorList');
        if (!sensorList) return;

        const selectedSensors = SensorSelectionManager.getSelectedSensors();
        sensorList.innerHTML = '';

        sensors.forEach(sensor => {
            const sensorCard = document.createElement('div');
            const isSelected = selectedSensors.length === 0 || 
                             selectedSensors.includes(sensor.address);

            sensorCard.className = `p-4 rounded-lg shadow cursor-pointer transition-all 
                ${isSelected ? 'bg-white' : 'bg-gray-100 opacity-50'}
                ${sensor.valid ? '' : 'bg-red-50'}
                hover:shadow-md`;
            
            sensorCard.innerHTML = `
                <div class="flex justify-between items-center">
                    <div class="font-medium text-gray-800">
                        ${sensor.name || `Sensor ${sensor.address.slice(-4)}`}
                    </div>
                    <div class="text-lg font-bold ${sensor.valid ? 'text-blue-600' : 'text-red-600'}">
                        ${sensor.valid ? sensor.temperature.toFixed(1) + '°C' : 'Error'}
                    </div>
                </div>
                ${!sensor.valid ? '<div class="text-xs text-red-500 mt-1">Invalid reading</div>' : ''}
            `;

            sensorCard.addEventListener('click', () => {
                const currentSelections = SensorSelectionManager.getSelectedSensors();
                let newSelections;
                
                if (currentSelections.includes(sensor.address)) {
                    newSelections = currentSelections.filter(addr => addr !== sensor.address);
                } else {
                    newSelections = [...currentSelections, sensor.address];
                }

                SensorSelectionManager.saveSelectedSensors(newSelections);
                this.updateSensorList(sensors);
                ChartUtils.updateChart();
            });

            sensorList.appendChild(sensorCard);
        });
    },

    updateCurrentSensor(sensors) {
        const currentTemperature = document.getElementById('currentTemperature');
        const currentSensorName = document.getElementById('currentSensorName');

        if (!currentTemperature || !currentSensorName) return;

        if (!AppState.selectedSensor) {
            currentTemperature.textContent = '--.-°C';
            currentSensorName.textContent = 'No sensor selected';
            return;
        }

        const currentSensor = sensors.find(s => s.address === AppState.selectedSensor);
        
        if (currentSensor && currentSensor.valid) {
            currentTemperature.textContent = `${currentSensor.temperature.toFixed(1)}°C`;
            currentSensorName.textContent = currentSensor.name || 
                                          `Sensor ${currentSensor.address.slice(-4)}`;
        }
    },

    updateSensorHistory(sensors) {
        sensors.forEach(sensor => {
            if (sensor.valid && sensor.temperature !== -127) {
                if (!AppState.sensorHistory[sensor.address]) {
                    AppState.sensorHistory[sensor.address] = [];
                }

                AppState.sensorHistory[sensor.address].push({
                    time: new Date(),
                    temp: sensor.temperature
                });

                // Limit history to last 20 entries
                if (AppState.sensorHistory[sensor.address].length > 20) {
                    AppState.sensorHistory[sensor.address].shift();
                }
            }
        });
    }
};
/**
 * Dashboard Initialization and Event Handling
 */
async function initializeDashboard() {
    Logger.log('Initializing Dashboard...');

    try {
        // Authentication check
        const isAuthenticated = await AuthUtils.checkAuth();
        if (!isAuthenticated) {
            Logger.error('Authentication failed');
            return;
        }

        // Initialize components
        const fetchSensors = SensorManager.fetchSensors.bind(SensorManager);
        await fetchSensors();

        // Initialize relay controls
        await RelayManager.initRelayControls();

        // Set up event listeners
        const setupEventListeners = () => {
            const ui = {
                sensorSelect: document.getElementById('sensorSelect'),
                refreshButton: document.getElementById('refreshButton'),
                logoutButton: document.getElementById('logoutButton')
            };

            if (ui.sensorSelect) {
                ui.sensorSelect.addEventListener('change', (e) => {
                    Logger.log('Sensor selection changed:', e.target.value);
                    AppState.selectedSensor = e.target.value;
                    fetchSensors();
                });
            } else {
                Logger.warn('Sensor select dropdown not found');
            }

            if (ui.refreshButton) {
                ui.refreshButton.addEventListener('click', () => {
                    Logger.log('Manual refresh triggered');
                    fetchSensors();
                    RelayManager.fetchRelayStates();
                });
            } else {
                Logger.warn('Refresh button not found');
            }

            if (ui.logoutButton) {
                ui.logoutButton.addEventListener('click', () => {
                    Logger.log('Logout initiated');
                    AuthUtils.logout();
                });
            } else {
                Logger.warn('Logout button not found');
            }
        };

        setupEventListeners();

        // Set up periodic refresh
        if (AppState.refreshInterval) {
            clearInterval(AppState.refreshInterval);
        }
        AppState.refreshInterval = setInterval(fetchSensors, 5000);

        // Initialize Lucide icons if available
        if (typeof lucide !== 'undefined' && lucide.createIcons) {
            lucide.createIcons();
            Logger.log('Lucide icons initialized');
        } else {
            Logger.warn('Lucide icons not available');
        }

    } catch (error) {
        Logger.error('Dashboard Initialization Failed', error);
        AuthUtils.showError('Failed to initialize dashboard');
    }
}
/**
 * Global Error Handling
 */
window.addEventListener('unhandledrejection', (event) => {
    Logger.error('Unhandled Promise Rejection', {
        reason: event.reason,
        promise: event.promise
    });
    AuthUtils.showError('An unexpected error occurred');
});
/**
 * Initialize Dashboard on DOM Load
 */
document.addEventListener('DOMContentLoaded', () => {
    Logger.log('DOM fully loaded, starting dashboard initialization');
    initializeDashboard();
});
