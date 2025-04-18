#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <rom/ets_sys.h>      // For ets_delay_us
#include "driver/gpio.h"      // Native GPIO functions
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// --- Extern declarations (likely internal/undocumented ESP-IDF APIs) ---
// Ensure these functions are linked properly (usually by including appropriate WiFi/BT components)
extern void rftest_init();
extern void phy_set_freq(unsigned int freq_mhz, int freq_khz);
extern void start_tx_tone(uint8_t p1_c_1, int sometimes_0, uint8_t att, uint8_t p4_c_0, int p5_c_0, uint8_t p6_c_0);
extern void force_txon(uint8_t onoff);

// --- Radio Initialization Function ---
void radio_init(void) {
    printf("Initializing RF test mode...\n");
    rftest_init();
    printf("RF test mode initialized.\n");
}

// --- Native GPIO RF Switch and External Antenna Configuration ---
// Configures GPIO3 (RF switch power ON, set LOW) and GPIO14 (select external antenna, set HIGH)
void configure_rf_switch_and_antenna_native(void) {
    gpio_set_direction(GPIO_NUM_3, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_3, 0);

    gpio_set_direction(GPIO_NUM_14, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_14, 1);

    printf("RF switch and external antenna configured with native GPIO functions.\n");
}

// --- Morse Code Table Definition ---
typedef struct {
    char symbol;
    const char *code;
} morse_entry_t;

static const morse_entry_t morse_table[] = {
    {'A', ".-"},   {'B', "-..."}, {'C', "-.-."}, {'D', "-.."},
    {'E', "."},    {'F', "..-."}, {'G', "--."},  {'H', "...."},
    {'I', ".."},   {'J', ".---"}, {'K', "-.-"},  {'L', ".-.."},
    {'M', "--"},   {'N', "-."},   {'O', "---"},  {'P', ".--."},
    {'Q', "--.-"}, {'R', ".-."},  {'S', "..."},  {'T', "-"},
    {'U', "..-"},  {'V', "...-"}, {'W', ".--"},  {'X', "-..-"},
    {'Y', "-.--"}, {'Z', "--.."},
    {'0', "-----"},{'1', ".----"},{'2', "..---"},{'3', "...--"},
    {'4', "....-"},{'5', "....."},{'6', "-...."},{'7', "--..."},
    {'8', "---.."},{'9', "----."},
};

// --- Lookup Function for Morse Code ---
// Returns the Morse code string for the given character (NULL if not found)
const char* lookup_morse(char c) {
    // Convert to uppercase if needed.
    if (c >= 'a' && c <= 'z') {
        c -= 32;
    }
    for (int i = 0; i < sizeof(morse_table)/sizeof(morse_table[0]); i++) {
        if (morse_table[i].symbol == c) {
            return morse_table[i].code;
        }
    }
    return NULL;
}

// --- Function to Transmit a Tone for a Given Duration ---
// Transmits a CW tone by turning the transmitter on for the specified duration (in milliseconds)
void send_morse_tone(uint8_t atten, uint32_t duration_ms) {
    force_txon(1);
    start_tx_tone(1, 0, atten, 0, 0, 0);
    ets_delay_us(duration_ms * 1000); // Convert ms to microseconds.
    force_txon(0);
}

// --- Function to Transmit a String as Morse Code ---
// Uses the "PARIS" standard where dot duration = 1200 / WPM (in ms).
// Includes detailed debug prints to monitor the state.
void send_morse_code(const char *text, uint8_t wpm, uint8_t atten) {
    double dot_duration_ms = 1200.0 / wpm;
    printf("\nStarting Morse Code Transmission\n");
    printf("Dot duration based on %u WPM: %.2f ms\n", wpm, dot_duration_ms);

    // Make a mutable copy of the text (strtok modifies the string)
    char *text_copy = strdup(text);
    if (!text_copy) {
        printf("Error: Unable to allocate memory for Morse code text copy.\n");
        return;
    }

    // Process each word separately.
    char *word = strtok(text_copy, " ");
    bool first_word = true;
    while (word != NULL) {
        // Word gap between words is 7 dot durations (except before the first word)
        if (!first_word) {
            ets_delay_us((uint32_t)(7 * dot_duration_ms * 1000));
        }
        printf("Sending word: %s\n", word);
        int word_len = strlen(word);
        for (int i = 0; i < word_len; i++) {
            char letter = word[i];
            printf("Sending letter: %c\n", letter);
            const char *morse = lookup_morse(letter);
            if (morse) {
                printf("Morse for %c: %s\n", letter, morse);
                int morse_len = strlen(morse);
                for (int j = 0; j < morse_len; j++) {
                    if (morse[j] == '.') {
                        send_morse_tone(atten, (uint32_t)dot_duration_ms);
                    } else if (morse[j] == '-') {
                        send_morse_tone(atten, (uint32_t)(3 * dot_duration_ms));
                    }
                    // Inter-element gap: 1 dot duration (between symbols of the same letter)
                    if (j < morse_len - 1) {
                        ets_delay_us((uint32_t)(dot_duration_ms * 1000));
                    }
                }
                // Letter gap: 3 dot durations between letters in the same word.
                if (i < word_len - 1) {
                    ets_delay_us((uint32_t)(3 * dot_duration_ms * 1000));
                }
            } else {
                printf("No Morse mapping for character: %c\n", letter);
            }
        }
        first_word = false;
        word = strtok(NULL, " ");
    }
    free(text_copy);
    printf("Morse code transmission cycle complete.\n");
}

// --- Main Application Entry Point ---
void app_main(void) {
    printf("ESP32 Morse Code CW Test\n");

    // Allow time for system initialization.
    vTaskDelay(pdMS_TO_TICKS(500));

    // Configure the RF switch and external antenna.
    configure_rf_switch_and_antenna_native();

    // Initialize the radio system.
    radio_init();

    // Optional delay after initialization.
    vTaskDelay(pdMS_TO_TICKS(100));

    // Set the transmission frequency (example: 2400 MHz with a 30 kHz offset).
    printf("Setting transmission frequency...\n");
    phy_set_freq(2400, 50);

    // Specify Morse code parameters.
    uint8_t wpm = 10;               // Initial Words Per Minute is set to 10.
    uint8_t atten = 0;              // Attenuation (0 means maximum power)
    const char *morse_text = "TEST TEST DE HB9BLA";

    // Transmit the Morse code repeatedly.
    while (1) {
        printf("\nTransmitting Morse code: \"%s\" at %u WPM\n", morse_text, wpm);
        send_morse_code(morse_text, wpm, atten);
        printf("Transmission finished. Waiting for 5 seconds before retransmission...\n");
        vTaskDelay(pdMS_TO_TICKS(5000)); // 5 second pause between transmissions.
    }
}