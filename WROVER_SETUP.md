# ESP32-WROVER-KIT Setup Guide

## ✅ Configuração Completa

O projeto agora está configurado para suportar **ESP32-WROVER-DEV V1.6**.

---

## 📋 Passos para Upload

### 1. Identifique a Porta COM

**Windows:**
```powershell
# Listar portas disponíveis
pio device list
```

Procure por algo como:
- `Silicon Labs CP210x` ou
- `USB-SERIAL CH340` ou  
- `COM3`, `COM4`, etc.

### 2. Atualize a Porta no platformio.ini

Edite a linha `upload_port` em `[env:esp32wrover]`:

```ini
upload_port = COM3  ; ← Substitua pela sua porta
```

### 3. Compile e Faça Upload

```bash
# Compilar para WROVER
pio run -e esp32wrover

# Upload
pio run -e esp32wrover --target upload

# Monitorar Serial
pio device monitor -e esp32wrover
```

---

## 🔧 Troubleshooting

### ❌ Erro: "Serial port not found"

**Solução:**
1. Instale drivers USB-to-Serial:
   - CP210x: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
   - CH340: http://www.wch.cn/downloads/CH341SER_EXE.html

2. Verifique a porta:
   ```bash
   pio device list
   ```

### ❌ Erro: "Timed out waiting for packet header"

**Solução - Modo Manual:**
1. **Segure o botão BOOT** no ESP32-WROVER
2. Execute: `pio run -e esp32wrover --target upload`
3. Quando aparecer "Connecting...", **solte o botão BOOT**

### ❌ Upload funciona mas nada aparece no Serial Monitor

**Solução:**
```bash
# Pressione o botão RESET físico no WROVER
# Ou reinicie com comando:
pio device monitor -e esp32wrover --echo --filter send_on_enter
# Digite: /reset
```

---

## 🎯 Diferenças WROVER vs ESP32-CAM

| Característica | ESP32-CAM | WROVER-KIT |
|----------------|-----------|------------|
| **PSRAM** | 4MB | **8MB** ✨ |
| **Resolução Padrão** | QVGA (320x240) | **SVGA (800x600)** ✨ |
| **Quality JPEG** | 18 | **12** (melhor) ✨ |
| **LED Flash** | ✅ GPIO 4 | ❌ Não disponível |
| **USB** | ❌ Requer FTDI | ✅ Embutido |
| **Upload** | Precisa apertar BOOT | **Automático** ✨ |

---

## 📊 Configurações Otimizadas

O código já está otimizado automaticamente:

### Para WROVER-KIT (8MB PSRAM):
```cpp
Resolution: SVGA (800x600)
JPEG Quality: 12 (alta qualidade)
Frame Buffers: 2
FPS Esperado: 15-20 FPS
```

### Para AI-Thinker (4MB PSRAM):
```cpp
Resolution: QVGA (320x240)
JPEG Quality: 18 (boa qualidade)
Frame Buffers: 2
FPS Esperado: 20-25 FPS
```

---

## 🚀 Comandos Rápidos

```bash
# Compilar WROVER
pio run -e esp32wrover

# Upload WROVER
pio run -e esp32wrover -t upload

# Monitor Serial
pio device monitor -e esp32wrover

# Limpar e Recompilar
pio run -e esp32wrover -t clean
pio run -e esp32wrover

# Upload + Monitor (tudo junto)
pio run -e esp32wrover -t upload && pio device monitor -e esp32wrover
```

---

## 🔍 Verificação Pós-Upload

Após upload bem-sucedido, você deve ver no Serial Monitor:

```
ESP32-CAM Robust Web Server
============================
Camera model: WROVER_KIT
PSRAM found, using resolution with 2 frame buffers
✓ Camera driver initialized
✓ Camera sensor acquired
Flushing initial frames...
  ✓ Flushed frame 1: 12345 bytes (800x600)
  ✓ Flushed frame 2: 12234 bytes (800x600)
  ...
Camera warmup complete (5/5 frames)
Applying custom camera settings...
✓ Settings applied
✓ Camera initialized successfully

WiFi Networks Found: 0
Starting WiFi in Captive Portal Mode...
AP SSID: ESP32-CAM-XXXXXX
AP IP: 192.168.4.1
✓ Captive Portal started
Starting HTTP Server...
✅ HTTP Server started successfully
   - /stream (MJPEG multipart)
   - /capture
   ...
```

---

## 🌐 Acessando a Câmera

1. **Conecte ao WiFi do ESP32:**
   - SSID: `ESP32-CAM-XXXXXX`
   - Senha: (nenhuma)

2. **Abra o navegador:**
   - URL: http://192.168.4.1

3. **Configure sua rede WiFi:**
   - Insira SSID e senha
   - Clique em "Connect to WiFi"

4. **Acesse o stream:**
   - URL: http://[IP-DO-ESP32]/stream
   - Você verá **800x600** em qualidade superior! 🎥

---

## 📝 Notas Importantes

- ✅ **WROVER-KIT não tem LED flash** - a funcionalidade está desabilitada automaticamente
- ✅ **Mais PSRAM = Melhor resolução** - aproveite os 8MB!
- ✅ **USB embutido** - muito mais fácil que ESP32-CAM
- ✅ **Código único** - funciona em ambas as placas automaticamente

---

## 🎯 Próximos Passos

Após configuração bem-sucedida:

1. ✅ Teste o stream em `/stream`
2. ✅ Configure WiFi permanente
3. ✅ Integre com Home Assistant
4. ✅ Experimente resoluções maiores (até UXGA 1600x1200)!

---

**Pronto para usar! 🚀**
