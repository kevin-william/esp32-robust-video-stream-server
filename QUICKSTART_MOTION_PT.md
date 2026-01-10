# Guia Rápido - Sensor de Movimento HC-SR501

## ⚡ Início Rápido (5 minutos)

### 1️⃣ Hardware
```
Conectar HC-SR501 ao ESP32-CAM:
  OUT → GPIO 33 (AI-Thinker) ou GPIO 12 (WROVER)
  VCC → 5V
  GND → GND
```

### 2️⃣ Preparação
```
1. Inserir cartão SD no ESP32-CAM
2. Ligar o dispositivo
3. Configurar WiFi via portal cativo (http://192.168.4.1)
```

### 3️⃣ Ativar Monitoramento
```bash
# Via navegador ou curl:
curl http://IP-DO-ESP32/motion/enable

# Reiniciar:
curl http://IP-DO-ESP32/restart
```

### 4️⃣ Testar
```
1. Mover mão na frente do sensor
2. Aguardar câmera ligar (LED pisca)
3. Esperar 5 segundos
4. Câmera desliga automaticamente
```

### 5️⃣ Ver Gravações
```
1. Remover cartão SD
2. Abrir pasta /recordings/
3. Arquivos: motion_XXXXX.mjpeg
4. Reproduzir com VLC Media Player
```

---

## 🎯 Como Funciona

```
SEM MOVIMENTO → Câmera DESLIGADA (economia de energia)
                      ↓
              MOVIMENTO DETECTADO!
                      ↓
              Câmera LIGA automaticamente
                      ↓
              Grava vídeo por 5 segundos
                      ↓
      Se houver novo movimento → Reinicia contador
                      ↓
      Sem movimento por 5s → Para gravação
                      ↓
              Câmera DESLIGA
                      ↓
              Volta ao início
```

---

## ⚙️ Configuração

### Via Arquivo (Recomendado)
Criar `/config/config.json` no cartão SD:
```json
{
  "motion": {
    "enabled": true,
    "recording_duration_sec": 5,
    "debounce_ms": 200
  }
}
```

### Via API
```bash
# Ver status
curl http://IP/motion/status

# Ativar
curl http://IP/motion/enable

# Desativar  
curl http://IP/motion/disable

# Aplicar mudanças
curl http://IP/restart
```

---

## 🔧 Ajuste do Sensor HC-SR501

### Potenciômetros
- **Sx** (Esquerdo): Sensibilidade (distância 3-7m)
  - Girar horário: mais sensível
  - Girar anti-horário: menos sensível
  
- **Tx** (Direito): Tempo de pulso (0.3s-5min)
  - Recomendado: 2-5 segundos
  - Girar horário: mais tempo
  - Girar anti-horário: menos tempo

### Jumper
- **H** (High): Modo repetível ✅ RECOMENDADO
  - Detecta movimento contínuo
  - Melhor para gravação
  
- **L** (Low): Modo único
  - Um trigger por movimento
  - Menos útil para este projeto

---

## 📊 Especificações

| Item | Valor |
|------|-------|
| Taxa de gravação | ~5 FPS |
| Duração padrão | 5 segundos |
| Formato | MJPEG |
| Tamanho arquivo | ~50-150 KB/s |
| Consumo idle | ~50mA (câmera OFF) |
| Consumo gravando | ~200mA (câmera ON) |
| Economia | ~100mA (66%) |

---

## 🎬 Formato de Vídeo

**MJPEG** = Sequência de imagens JPEG
- ✅ Leve e otimizado
- ✅ Funciona em VLC
- ✅ Fácil conversão

### Converter para MP4
```bash
ffmpeg -i motion_123456.mjpeg -c:v libx264 -r 5 video.mp4
```

---

## 🐛 Solução de Problemas

### Sensor não detecta movimento
```
✓ Esperar 30-60s após ligar (aquecimento)
✓ Ajustar potenciômetro Sx (sensibilidade)
✓ Verificar jumper em posição H
✓ Conferir conexões (VCC, GND, OUT)
```

### Sem gravações no SD
```
✓ Verificar se SD está montado: /status
✓ Verificar espaço livre no SD
✓ Ver logs serial para erros
✓ Tentar formatar SD (FAT32)
```

### Muitos falsos triggers
```
✓ Aumentar debounce_ms (ex: 500)
✓ Reduzir sensibilidade Sx
✓ Afastar de fontes de calor
✓ Evitar luz solar direta
```

### Câmera não inicia
```
✓ Testar câmera em modo normal primeiro
✓ Verificar alimentação 5V 2A
✓ Ver logs serial
✓ Tentar reiniciar dispositivo
```

---

## 📱 Endpoints API

### Status Geral
```bash
GET http://IP/status

Resposta:
{
  "camera_initialized": false,
  "motion_monitoring_enabled": true,
  "motion_monitoring_active": true,
  "sd_card_mounted": true,
  ...
}
```

### Status de Movimento
```bash
GET http://IP/motion/status

Resposta:
{
  "motion_monitoring_enabled": true,
  "motion_monitoring_active": true,
  "motion_recording_active": false,
  "sd_card_mounted": true,
  "recording_duration_sec": 5,
  "time_since_last_motion_ms": 1234
}
```

### Ativar/Desativar
```bash
# Ativar (requer SD + reinício)
GET http://IP/motion/enable

# Desativar (requer reinício)
GET http://IP/motion/disable
```

---

## 📚 Documentação Completa

Para mais detalhes, consulte:

- 🇬🇧 **Inglês**: [docs/MOTION_SENSOR.md](docs/MOTION_SENSOR.md)
- 🇧🇷 **Português**: [docs/MOTION_SENSOR_PT.md](docs/MOTION_SENSOR_PT.md)
- 🧪 **Testes**: [docs/TESTING_MOTION_SENSOR.md](docs/TESTING_MOTION_SENSOR.md)
- 🏗️ **Arquitetura**: [docs/ARCHITECTURE_MOTION.md](docs/ARCHITECTURE_MOTION.md)
- 📋 **Resumo**: [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)

---

## ✅ Checklist de Instalação

- [ ] HC-SR501 conectado ao GPIO correto
- [ ] Cartão SD inserido e formatado (FAT32)
- [ ] WiFi configurado
- [ ] Motion monitoring ativado via API
- [ ] Dispositivo reiniciado
- [ ] Teste de movimento realizado
- [ ] Vídeo gravado e reproduzido

---

## 💡 Dicas

1. **Primeiro teste sem motion**: Verifique que câmera funciona normalmente
2. **Aguarde aquecimento**: Sensor precisa 30-60s após ligar
3. **Use SD rápido**: Cartão Class 10 ou superior
4. **Monitore serial**: Logs ajudam muito no debug
5. **Teste gradual**: Um passo por vez

---

## 🎯 Uso Típico

### Segurança Residencial
```
1. Posicionar ESP32-CAM na entrada
2. Ajustar sensor para cobrir área
3. Gravar quando alguém se aproxima
4. Revisar gravações periodicamente
```

### Monitoramento de Animais
```
1. Colocar próximo a comedouro
2. Gravar quando animal se alimenta
3. Estudar comportamento
```

### Time-lapse de Movimento
```
1. Gravar apenas quando há atividade
2. Economizar espaço no SD
3. Criar compilação depois
```

---

## 🚀 Pronto!

Agora seu ESP32-CAM está configurado para:
- ✅ Detectar movimento automaticamente
- ✅ Gravar vídeos quando necessário
- ✅ Economizar energia quando ocioso
- ✅ Armazenar tudo no cartão SD

**Divirta-se!** 🎉

---

Para suporte adicional:
- 📧 GitHub Issues
- 📚 Documentação completa
- 💬 Comunidade ESP32

**Versão**: 1.0  
**Data**: Janeiro 2026  
**Licença**: Apache-2.0
