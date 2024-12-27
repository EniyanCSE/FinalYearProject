// ESP32-CAM Code
#include <WiFi.h>
#include <esp_camera.h>
#include <WebServer.h>
#include <DustbinFinalDataModel_inferencing.h>

// Wi-Fi credentials
const char* ssid = "";  // Replace with your Wi-Fi SSID
const char* password = "";  // Replace with your Wi-Fi password

// Select camera model
#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27
#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22
#else
#error "Camera model not selected"
#endif

WebServer server(80); // Start a web server on port 80
camera_fb_t* fb;      // Frame buffer for the camera

String raspberryPiURL = "http://<raspberry_pi_ip>:5000/process"; // Raspberry Pi URL
bool waitingForPi = false;

void connectToWiFi() {
    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting...");
    }
    Serial.println("Connected to Wi-Fi");
    Serial.println(WiFi.localIP());
}

void sendImageToRaspberryPi(camera_fb_t* fb) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, raspberryPiURL);
    http.addHeader("Content-Type", "image/jpeg");
    int httpResponseCode = http.POST(fb->buf, fb->len);

    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Response from Raspberry Pi: " + response);
        // Handle response
    } else {
        Serial.println("Failed to connect to Raspberry Pi");
    }

    http.end();
}

bool runInferenceOnDevice(camera_fb_t* fb) {
    ei_impulse_result_t result = { 0 };
    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
    if (err != EI_IMPULSE_OK) {
        Serial.printf("Classifier error: %d\n", err);
        return false;
    }

    float highest_confidence = 0.0;
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > highest_confidence) {
            highest_confidence = result.classification[i].value;
        }
    }

    Serial.printf("Highest confidence: %.2f\n", highest_confidence);
    return highest_confidence >= 0.85;
}

void captureAndProcessImage() {
    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }

    if (!runInferenceOnDevice(fb)) {
        Serial.println("Confidence below threshold, sending image to Raspberry Pi...");
        sendImageToRaspberryPi(fb);
        waitingForPi = true;
    } else {
        Serial.println("Image processed locally with sufficient confidence.");
    }

    esp_camera_fb_return(fb);
}

void setup() {
    Serial.begin(115200);
    connectToWiFi();

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    if (esp_camera_init(&config) != ESP_OK) {
        Serial.println("Camera init failed");
        return;
    }

    captureAndProcessImage();
}

void loop() {
    if (waitingForPi) {
        // Continue waiting for Raspberry Pi response
    }
}
