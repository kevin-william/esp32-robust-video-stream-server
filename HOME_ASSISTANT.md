# Home Assistant Integration

## Camera Configuration

Este ESP32-CAM server usa **Motion-JPEG via snapshot polling** para compatibilidade com Home Assistant.

### Configuração no Home Assistant

Adicione ao seu `configuration.yaml`:

```yaml
camera:
  - platform: generic
    name: ESP32-CAM
    still_image_url: http://192.168.0.152/capture
    verify_ssl: false
    framerate: 10
```

**Substitua `192.168.0.152` pelo IP do seu ESP32-CAM.**

### Por que não usar MJPEG multipart?

AsyncWebServer (biblioteca usada) não suporta streaming MJPEG multipart contínuo.
A solução com `generic` platform + `still_image_url` funciona perfeitamente e é mais eficiente.

### Endpoints Disponíveis

- **Stream MJPEG**: `http://<ESP32-IP>/stream`
  - Formato: multipart/x-mixed-replace
  - Taxa: ~10 FPS
  - Uso: Home Assistant, navegadores

- **Captura única**: `http://<ESP32-IP>/capture`
  - Formato: image/jpeg
  - Uso: Snapshot, notificações

- **Diagnósticos**: `http://<ESP32-IP>/diagnostics`
  - Formato: application/json
  - Retorna: Status do sistema, memória, WiFi, streaming

### Exemplo de Automação

```yaml
automation:
  - alias: "Notificar movimento detectado"
    trigger:
      - platform: state
        entity_id: binary_sensor.movimento_sala
        to: "on"
    action:
      - service: notify.mobile_app
        data:
          message: "Movimento detectado!"
          data:
            image: "http://192.168.0.152/capture"
```

### Cartão Lovelace

```yaml
type: picture-glance
title: ESP32-CAM
camera_image: camera.esp32_cam
entities:
  - camera.esp32_cam
camera_view: live
```

### Troubleshooting

**Stream não funciona:**
1. Verifique se a câmera está acessível: `http://<IP>/diagnostics`
2. Teste o endpoint direto no navegador: `http://<IP>/stream`
3. Verifique logs do ESP32 via serial monitor

**Qualidade baixa:**
- Acesse `http://<IP>/` para ajustar configurações da câmera
- Ajuste resolução, qualidade JPEG, brilho, etc.

**FPS baixo:**
- Normal para ESP32-CAM (limitado por hardware)
- Configure `frame_rate` no Home Assistant para 10 FPS
- Use WiFi 2.4GHz com bom sinal

## API REST

### GET /control
Controlar configurações da câmera:
```
http://<IP>/control?var=framesize&val=8
http://<IP>/control?var=quality&val=10
http://<IP>/control?var=brightness&val=1
```

### POST /wifi-connect
Conectar a nova rede WiFi (modo captive portal).

### GET /diagnostics
Retorna JSON com status completo do sistema.

### GET /restart
Reinicia o ESP32.
