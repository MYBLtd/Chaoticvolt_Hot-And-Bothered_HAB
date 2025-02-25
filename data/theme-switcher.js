const ThemeSwitcher = {
    // Theme keys
    THEME_KEY: 'sensorhub_theme',
    DARK_THEME: 'dark',
    LIGHT_THEME: 'light',
    
    // Initialize theme on page load
    init() {
        // Set the theme based on user preference or system preference
        this.applyTheme(this.getThemePreference());
        
        // Look for all theme toggle buttons and attach listeners
        const toggleButtons = document.querySelectorAll('[data-theme-toggle]');
        toggleButtons.forEach(button => {
            button.addEventListener('click', () => this.toggleTheme());
        });
    },
    
    // Get the current theme preference from localStorage or system preference
    getThemePreference() {
        // Check if user has a saved preference
        const savedTheme = localStorage.getItem(this.THEME_KEY);
        if (savedTheme) {
            return savedTheme;
        }
        
        // If no saved preference, check system preference
        if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
            return this.DARK_THEME;
        }
        
        // Default to light theme
        return this.LIGHT_THEME;
    },
    
    // Toggle between light and dark themes
    toggleTheme() {
        const currentTheme = this.getThemePreference();
        const newTheme = currentTheme === this.LIGHT_THEME ? this.DARK_THEME : this.LIGHT_THEME;
        
        // Save the new preference
        localStorage.setItem(this.THEME_KEY, newTheme);
        
        // Apply the new theme
        this.applyTheme(newTheme);
        
        // Update toggle button icons
        this.updateToggleIcons(newTheme);
    },
    
    // Apply the specified theme to the document
    applyTheme(theme) {
        if (theme === this.DARK_THEME) {
            document.documentElement.classList.add('dark');
        } else {
            document.documentElement.classList.remove('dark');
        }
        
        // Update toggle button icons based on current theme
        this.updateToggleIcons(theme);
        
        // Update chart theme if ChartUtils exists
        if (typeof ChartUtils !== 'undefined' && ChartUtils.updateChartTheme) {
            ChartUtils.updateChartTheme();
        }
    },
    
    // Update all toggle button icons to reflect current theme
    updateToggleIcons(theme) {
        const toggleButtons = document.querySelectorAll('[data-theme-toggle]');
        
        toggleButtons.forEach(button => {
            // Find the icon elements
            const moonIcon = button.querySelector('[data-lucide="moon"]');
            const sunIcon = button.querySelector('[data-lucide="sun"]');
            
            if (moonIcon && sunIcon) {
                if (theme === this.DARK_THEME) {
                    moonIcon.classList.add('hidden');
                    sunIcon.classList.remove('hidden');
                } else {
                    moonIcon.classList.remove('hidden');
                    sunIcon.classList.add('hidden');
                }
            }
        });
    }
};

// Initialize the theme switcher when the DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    ThemeSwitcher.init();
});