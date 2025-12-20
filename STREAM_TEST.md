# Problema com AsyncWebServer e MJPEG

## O Problema

`AsyncWebServer::beginChunkedResponse()` **NÃO** foi projetado para streaming MJPEG contínuo infinito. 

O callback é chamado **apenas uma vez** ou poucas vezes, não continuamente.

## Soluções Possíveis

### Opção 1: Usar WebServer síncrono (oficial)
- Trocar AsyncWebServer por WebServer padrão
- Usar código do exemplo CameraWebServer oficial
- **Desvantagem**: Menos eficiente, bloqueia durante requests

### Opção 2: Motion-JPEG via polling (atual)
- JavaScript faz refresh de `/capture` continuamente
- Simula stream visual
- **Funciona**, mas usa mais banda

### Opção 3: WebSocket com frames JPEG
- Enviar JPEGs via WebSocket
- Renderizar no canvas JavaScript
- Mais controle, melhor performance

## Recomendação

Para Home Assistant e uso geral, a melhor solução é usar **Motion-JPEG polling** que já está implementado no JavaScript.

Para o navegador, funciona perfeitamente bem. Home Assistant também suporta snapshot_url com refresh.
