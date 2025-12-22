# ESP32-WROVER-DEV V1.6 Debug Guide

## Informações do Hardware
- Board: ESP32-WROVER-DEV V1.6
- Camera: OV2640 via flat cable
- PSRAM: 8MB

## Configuração Atual dos Pinos

A configuração atual em `camera_pins.h` está usando os pinos do **AI-Thinker** (mais comum em clones):

```cpp
PWDN_GPIO_NUM     32
RESET_GPIO_NUM    -1
XCLK_GPIO_NUM     0
SIOD_GPIO_NUM     26  // I2C SDA
SIOC_GPIO_NUM     27  // I2C SCL

Y9_GPIO_NUM       35
Y8_GPIO_NUM       34
Y7_GPIO_NUM       39
Y6_GPIO_NUM       36
Y5_GPIO_NUM       21
Y4_GPIO_NUM       19
Y3_GPIO_NUM       18
Y2_GPIO_NUM       5
VSYNC_GPIO_NUM    25
HREF_GPIO_NUM     23
PCLK_GPIO_NUM     22
```

## Códigos de Erro Comuns

| Código | Significado | Possível Causa |
|--------|-------------|----------------|
| 0x105 | ESP_ERR_NOT_FOUND | Sensor não detectado via I2C |
| 0x20001 | Falha I2C | Pinos SIOD/SIOC incorretos |
| 0x107 | Timeout I2C | Câmera não energizada ou cabo solto |
| 0x103 | ESP_ERR_INVALID_ARG | Configuração de pino inválida |

## Passos de Diagnóstico

### 1. Compilar e Enviar
```bash
# No PlatformIO:
pio run -e esp32wrover -t upload

# Ou no Arduino IDE:
# - Selecione Board: "ESP32 Wrover Module"
# - Upload Speed: 115200
# - Flash Frequency: 80MHz
# - Partition Scheme: "Huge APP (3MB No OTA)"
```

### 2. Abrir Serial Monitor
- Velocidade: **115200 baud**
- Observe as mensagens de inicialização

### 3. Mensagens Esperadas

**Se a câmera inicializar corretamente:**
```
Camera Initialization
======================
Camera Model: WROVER_KIT
Pin Configuration:
  PWDN    : GPIO 32
  XCLK    : GPIO 0
  SIOD    : GPIO 26
  SIOC    : GPIO 27
  [...]
Calling esp_camera_init()...
esp_camera_init() returned: 0x0
✓ Camera initialized successfully!
```

**Se falhar:**
```
Camera Initialization
======================
Camera Model: WROVER_KIT
Pin Configuration:
  PWDN    : GPIO 32
  [...]
Calling esp_camera_init()...
esp_camera_init() returned: 0x105
❌ Camera init failed with error 0x105
  Error: ESP_ERR_NOT_FOUND - Camera sensor not detected
  Possible causes:
    - Camera not connected properly
    - Wrong pin configuration
    - Camera power issue
```

## Configurações Alternativas para Testar

### Se a configuração AI-Thinker não funcionar, tente:

#### Opção A: Pinos Oficiais WROVER-KIT
Edite `include/camera_pins.h` linha ~50:
```cpp
#elif defined(CAMERA_MODEL_WROVER_KIT)
    #define PWDN_GPIO_NUM     -1
    #define RESET_GPIO_NUM    -1
    #define XCLK_GPIO_NUM     21
    #define SIOD_GPIO_NUM     26
    #define SIOC_GPIO_NUM     27

    #define Y9_GPIO_NUM       35
    #define Y8_GPIO_NUM       34
    #define Y7_GPIO_NUM       39
    #define Y6_GPIO_NUM       36
    #define Y5_GPIO_NUM       19
    #define Y4_GPIO_NUM       18
    #define Y3_GPIO_NUM       5
    #define Y2_GPIO_NUM       4
    #define VSYNC_GPIO_NUM    25
    #define HREF_GPIO_NUM     23
    #define PCLK_GPIO_NUM     22
```

#### Opção B: Pinos M5Stack
Se sua placa for semelhante ao M5Stack:
```cpp
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    15
#define XCLK_GPIO_NUM     27
#define SIOD_GPIO_NUM     25
#define SIOC_GPIO_NUM     23

#define Y9_GPIO_NUM       19
#define Y8_GPIO_NUM       36
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       39
#define Y5_GPIO_NUM       5
#define Y4_GPIO_NUM       34
#define Y3_GPIO_NUM       35
#define Y2_GPIO_NUM       32
#define VSYNC_GPIO_NUM    22
#define HREF_GPIO_NUM     26
#define PCLK_GPIO_NUM     21
```

## Verificação do Flat Cable

1. **Orientação**: O lado azul do cabo deve estar voltado para os chips na placa
2. **Conexão**: Os pinos dourados devem estar totalmente inseridos no conector
3. **Trava**: A trava preta do conector deve estar fechada firmemente

## Teste de Alimentação

A câmera OV2640 precisa de:
- **3.3V**: Fornecido pela placa
- **Corrente**: ~100mA durante operação
- **Verificar**: LED de alimentação da câmera (se disponível)

## Informações para Reportar

Se ainda não funcionar, forneça:
1. **Log serial completo** desde o boot até a falha
2. **Código de erro** exato (ex: 0x105)
3. **Foto** da conexão do flat cable (se possível)
4. **Modelo exato** impresso na placa WROVER
5. **Tensão** medida no pino 3.3V (se tiver multímetro)

## Links Úteis

- [ESP32-CAM Pinout Reference](https://randomnerdtutorials.com/esp32-cam-ai-thinker-pinout/)
- [OV2640 Datasheet](https://www.uctronics.com/download/cam_module/OV2640DS.pdf)
- [ESP-IDF Camera Driver](https://github.com/espressif/esp32-camera)
