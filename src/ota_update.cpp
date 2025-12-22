#include "ota_update.h"
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_log.h>
#include <esp_system.h>
#include "ArduinoJson.h"

static const char* TAG = "OTA";

// OTA state
static esp_ota_handle_t ota_handle = 0;
static const esp_partition_t *update_partition = NULL;
static char ota_status[128] = "Ready";
static int ota_progress = 0;
static size_t bytes_received = 0;
static size_t total_size = 0;

void initOTA() {
    // Get the partition info
    const esp_partition_t *running = esp_ota_get_running_partition();
    update_partition = esp_ota_get_next_update_partition(NULL);
    
    if (update_partition != NULL) {
        ESP_LOGI(TAG, "Running partition: %s at 0x%lx", 
                 running->label, running->address);
        ESP_LOGI(TAG, "Update partition: %s at 0x%lx", 
                 update_partition->label, update_partition->address);
        snprintf(ota_status, sizeof(ota_status), "Ready - Update partition: %s", 
                 update_partition->label);
    } else {
        ESP_LOGE(TAG, "No OTA update partition found!");
        snprintf(ota_status, sizeof(ota_status), "Error: No OTA partition");
    }
}

const char* getOTAStatus() {
    return ota_status;
}

int getOTAProgress() {
    return ota_progress;
}

// Handler for /update page (HTML interface)
static esp_err_t handleUpdatePage(httpd_req_t *req) {
    const char* html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-CAM OTA Update</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 600px;
            margin: 50px auto;
            padding: 20px;
            background: #f0f0f0;
        }
        .container {
            background: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
        }
        .status {
            padding: 15px;
            margin: 20px 0;
            border-radius: 5px;
            background: #e3f2fd;
            border-left: 4px solid #2196f3;
        }
        .warning {
            background: #fff3cd;
            border-left-color: #ffc107;
        }
        .error {
            background: #f8d7da;
            border-left-color: #dc3545;
        }
        .success {
            background: #d4edda;
            border-left-color: #28a745;
        }
        input[type="file"] {
            display: block;
            width: 100%;
            padding: 10px;
            margin: 20px 0;
            border: 2px dashed #ccc;
            border-radius: 5px;
            cursor: pointer;
        }
        button {
            background: #2196f3;
            color: white;
            border: none;
            padding: 12px 30px;
            font-size: 16px;
            border-radius: 5px;
            cursor: pointer;
            width: 100%;
        }
        button:hover {
            background: #1976d2;
        }
        button:disabled {
            background: #ccc;
            cursor: not-allowed;
        }
        .progress-container {
            display: none;
            margin: 20px 0;
        }
        .progress-bar {
            width: 100%;
            height: 30px;
            background: #f0f0f0;
            border-radius: 15px;
            overflow: hidden;
        }
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #2196f3, #21d4f3);
            width: 0%;
            transition: width 0.3s;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-weight: bold;
        }
        .info {
            font-size: 14px;
            color: #666;
            margin-top: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔄 Firmware Update</h1>
        
        <div id="status-box" class="status">
            <strong>Status:</strong> <span id="status-text">Ready</span>
        </div>
        
        <div class="status warning">
            <strong>⚠️ Atenção:</strong>
            <ul>
                <li>Não desligue o dispositivo durante a atualização</li>
                <li>Certifique-se de que o arquivo .bin é válido</li>
                <li>A atualização leva cerca de 30-60 segundos</li>
            </ul>
        </div>
        
        <form id="upload-form">
            <input type="file" id="file-input" accept=".bin" required>
            <div class="info">
                Selecione o arquivo firmware.bin
            </div>
            
            <div class="progress-container" id="progress-container">
                <div class="progress-bar">
                    <div class="progress-fill" id="progress-fill">0%</div>
                </div>
            </div>
            
            <button type="submit" id="upload-btn">Upload Firmware</button>
        </form>
        
        <div class="info" style="margin-top: 30px; text-align: center;">
            <a href="/">← Voltar para página principal</a>
        </div>
    </div>
    
    <script>
        const form = document.getElementById('upload-form');
        const fileInput = document.getElementById('file-input');
        const uploadBtn = document.getElementById('upload-btn');
        const statusBox = document.getElementById('status-box');
        const statusText = document.getElementById('status-text');
        const progressContainer = document.getElementById('progress-container');
        const progressFill = document.getElementById('progress-fill');
        
        function setStatus(message, type) {
            statusText.textContent = message;
            statusBox.className = 'status ' + type;
        }
        
        function updateProgress(percent) {
            progressFill.style.width = percent + '%';
            progressFill.textContent = percent + '%';
        }
        
        form.addEventListener('submit', async (e) => {
            e.preventDefault();
            
            const file = fileInput.files[0];
            if (!file) {
                setStatus('Selecione um arquivo', 'error');
                return;
            }
            
            if (!file.name.endsWith('.bin')) {
                setStatus('Arquivo deve ter extensão .bin', 'error');
                return;
            }
            
            uploadBtn.disabled = true;
            fileInput.disabled = true;
            progressContainer.style.display = 'block';
            setStatus('Enviando firmware...', '');
            
            try {
                const xhr = new XMLHttpRequest();
                
                xhr.upload.addEventListener('progress', (e) => {
                    if (e.lengthComputable) {
                        const percent = Math.round((e.loaded / e.total) * 100);
                        updateProgress(percent);
                    }
                });
                
                xhr.addEventListener('load', () => {
                    if (xhr.status === 200) {
                        setStatus('✅ Atualização concluída! Reiniciando...', 'success');
                        setTimeout(() => {
                            window.location.href = '/';
                        }, 5000);
                    } else {
                        setStatus('❌ Erro: ' + xhr.responseText, 'error');
                        uploadBtn.disabled = false;
                        fileInput.disabled = false;
                    }
                });
                
                xhr.addEventListener('error', () => {
                    setStatus('❌ Erro na conexão', 'error');
                    uploadBtn.disabled = false;
                    fileInput.disabled = false;
                });
                
                xhr.open('POST', '/update/upload');
                xhr.send(file);
                
            } catch (error) {
                setStatus('❌ Erro: ' + error.message, 'error');
                uploadBtn.disabled = false;
                fileInput.disabled = false;
            }
        });
    </script>
</body>
</html>
)rawliteral";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}

// Handler for /update/upload (receives firmware binary)
static esp_err_t handleUpdateUpload(httpd_req_t *req) {
    esp_err_t err;
    char buf[1024];
    size_t received = 0;
    int remaining = req->content_len;
    bool first_chunk = true;
    
    ESP_LOGI(TAG, "Starting OTA update, size: %d bytes", remaining);
    
    while (remaining > 0) {
        int recv_len = httpd_req_recv(req, buf, min(remaining, (int)sizeof(buf)));
        
        if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        } else if (recv_len <= 0) {
            ESP_LOGE(TAG, "Connection error");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Connection error");
            if (ota_handle) {
                esp_ota_abort(ota_handle);
                ota_handle = 0;
            }
            return ESP_FAIL;
        }
        
        // First chunk - begin OTA
        if (first_chunk) {
            update_partition = esp_ota_get_next_update_partition(NULL);
            if (update_partition == NULL) {
                ESP_LOGE(TAG, "No OTA partition found");
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition");
                return ESP_FAIL;
            }
            
            err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA begin failed");
                return ESP_FAIL;
            }
            
            first_chunk = false;
            bytes_received = 0;
            total_size = req->content_len;
            snprintf(ota_status, sizeof(ota_status), "Uploading...");
        }
        
        // Write chunk
        err = esp_ota_write(ota_handle, (const void *)buf, recv_len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
            esp_ota_abort(ota_handle);
            ota_handle = 0;
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA write failed");
            return ESP_FAIL;
        }
        
        received += recv_len;
        remaining -= recv_len;
        
        ota_progress = (received * 100) / total_size;
        
        if (received % 10240 == 0) {  // Log every 10KB
            ESP_LOGI(TAG, "Written %d/%d bytes (%d%%)", received, total_size, ota_progress);
        }
    }
    
    ESP_LOGI(TAG, "OTA upload complete, verifying...");
    
    // Finish OTA
    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA verification failed");
        ota_handle = 0;
        return ESP_FAIL;
    }
    
    // Set boot partition
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Set boot partition failed");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "OTA update successful! Rebooting...");
    snprintf(ota_status, sizeof(ota_status), "Update successful, rebooting...");
    
    httpd_resp_sendstr(req, "Update successful! Rebooting...");
    
    // Reboot after 2 seconds
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
    
    return ESP_OK;
}

void registerOTAEndpoints(httpd_handle_t server) {
    // Register /update page
    httpd_uri_t update_page_uri = {
        .uri       = "/update",
        .method    = HTTP_GET,
        .handler   = handleUpdatePage,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &update_page_uri);
    
    // Register /update/upload endpoint
    httpd_uri_t update_upload_uri = {
        .uri       = "/update/upload",
        .method    = HTTP_POST,
        .handler   = handleUpdateUpload,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &update_upload_uri);
    
    ESP_LOGI(TAG, "OTA endpoints registered");
}
