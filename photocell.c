
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#define RED_PIN GPIO_NUM_8
#define GREEN_PIN GPIO_NUM_6
#define BLUE_PIN GPIO_NUM_7
// Function to setup GPIO for RGB LED
void setup_gpio() {
    esp_rom_gpio_pad_select_gpio(RED_PIN);
    gpio_set_direction(RED_PIN, GPIO_MODE_OUTPUT);

    esp_rom_gpio_pad_select_gpio(GREEN_PIN);
    gpio_set_direction(GREEN_PIN, GPIO_MODE_OUTPUT);

    esp_rom_gpio_pad_select_gpio(BLUE_PIN);
    gpio_set_direction(BLUE_PIN, GPIO_MODE_OUTPUT);
}

void set_led_colour(bool red, bool green, bool blue) {
    gpio_set_level(RED_PIN, red ? 1 : 0);
    gpio_set_level(GREEN_PIN, green ? 1 : 0);
    gpio_set_level(BLUE_PIN, blue ? 1 : 0);
}
// Function to setup ADC for photocell
void setup_adc(adc_oneshot_unit_handle_t *adc_handle) {
    adc_oneshot_unit_init_cfg_t adc_config = {.unit_id = ADC_UNIT_1};
    esp_err_t err = adc_oneshot_new_unit(&adc_config, adc_handle);
    if (err != ESP_OK) {
        printf("Error setting up ADC: %x\n", err);
        return;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_0,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };
    err = adc_oneshot_config_channel(*adc_handle, ADC_CHANNEL_0, &chan_cfg);
    if (err != ESP_OK) {
        printf("Error configuring ADC channel: %x\n", err);
    }
}

// Function to read value from photocell
int analog_read(adc_oneshot_unit_handle_t handle) {
    int adc_r;
    esp_err_t err = adc_oneshot_read(handle, ADC_CHANNEL_0, &adc_r);
    if (err != ESP_OK) {
        printf("ERR: ADC Measurement failed: %x\n", err);
        return -1; // Indicates an error
    }
    return adc_r; // Return the ADC reading
}


// Main application entry point
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#define BUZZER_PIN GPIO_NUM_9  // Choose a PWM-capable pin

#define BUZZER_CHANNEL LEDC_CHANNEL_0
#define BUZZER_FREQ    2000     // PWM frequency in Hz
#define BUZZER_RES     LEDC_TIMER_13_BIT


//Set up the LEDC module of the ESP32-C3 to generate a PWM signal.
//The PWM signal is used to change the tone and frequency of the output sound*/

void setup_pwm() {
    //Set up the LEDC timer
    ledc_timer_config_t timer_conf = {
        .duty_resolution = BUZZER_RES,
        .freq_hz = BUZZER_FREQ,
        .speed_mode = LEDC_TIMER_0,
        .timer_num = LEDC_TIMER_0
    };
    ledc_timer_config(&timer_conf);

    //Set up the LEDC output channel
    ledc_channel_config_t channel_conf = {
        .gpio_num = BUZZER_PIN,
        .speed_mode = LEDC_TIMER_0,
        .channel = BUZZER_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0
    };
    ledc_channel_config(&channel_conf);
}

//Play a tone with a given frequency (in hz) for a period of time (duration = milliseconds)
void play_tone(int frequency, int duration) {
    ledc_set_freq(LEDC_TIMER_0, LEDC_TIMER_0, frequency);
    ledc_set_duty(LEDC_TIMER_0, BUZZER_CHANNEL, 50);
    ledc_update_duty(LEDC_TIMER_0, BUZZER_CHANNEL);

    vTaskDelay(duration / portTICK_PERIOD_MS);

    ledc_set_duty(LEDC_TIMER_0, BUZZER_CHANNEL, 0);
    ledc_update_duty(LEDC_TIMER_0, BUZZER_CHANNEL);
}

//Play a nice little melody
void play_melody() {
  //The frequencies
    int melody[] = {
        392, 392, 392
    };

    //The duration of each note
    int note_durations[] = {
         200, 200, 200
    };

     for (int i = 0; i < 3; ++i) {
        printf("play tone %d",i);
        play_tone(melody[i], note_durations[i]);
        vTaskDelay(50 / portTICK_PERIOD_MS); // Delay between notes
    }
}

// Function declarations
void setup_gpio();
void set_led_colour(bool red, bool green, bool blue);
int analog_read(adc_oneshot_unit_handle_t handle);
void setup_adc(adc_oneshot_unit_handle_t *adc_handle);

void app_main() {
    setup_pwm(); // Initialize the PWM for the buzzer
    setup_gpio(); // Initialize the GPIO pins for LED

    // Setup ADC for photocell
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t adc_config = {.unit_id = ADC_UNIT_1};
    esp_err_t err = adc_oneshot_new_unit(&adc_config, &adc_handle);
    if (err != ESP_OK) {
        printf("Error setting up ADC: %x\n", err);
        return;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_0,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };
    err = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &chan_cfg);
    if (err != ESP_OK) {
        printf("Error configuring ADC channel: %x\n", err);
        return;
    }

    while (1) {
        printf("Checking light level...\n"); // Debug print
        int light_level = analog_read(adc_handle); // Read light level from photocell
        printf("Light level: %d\n", light_level); // Confirm light level reading

        // Determine the color and print statements based on light level
        if (light_level < 1350) {  // Low illumination
            set_led_colour(false, true, true); // Red LED
            printf("LED colour: Red (Illumination: Low)\n");
            play_melody(); // Continuously play the melody
        } else if (light_level < 2700) {  // Moderate illumination
            set_led_colour(true, false, true); // Green LED
            printf("LED colour: Green (Illumination: Moderate)\n");
        } else {  // High illumination
            set_led_colour(true, true, false); // Blue LED
            printf("LED colour: Blue (Illumination: High)\n");
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);  // Delay for 2 seconds
    }
}

