# Integración pendiente con StackChan CoreS3

Estado: el procesador de `meeting_assistant` acepta un archivo existente, pero no captura audio del robot ni se conecta aún a la VPS, OpenClaw o Xiaozhi. No hay firmware de reuniones compilado o validado.

## Hallazgos verificables

- Firmware oficial: ESP-IDF 5.5.4, objetivo ESP32-S3, audio de entrada a 24 kHz. `firmware/main/hal/audio.cpp` demuestra la lectura del códec durante la prueba del micrófono.
- La documentación de M5Stack asigna a microSD CS=4, SCK=36, MISO=35 y MOSI=37. El firmware usa SPI3 para la pantalla en SCK=36 y MOSI=37, con GPIO35 como DC de pantalla. Cualquier soporte de microSD debe comprobar el manejo compartido del bus y de ese pin en el hardware real; no basta con inicializar un segundo bus sin coordinarlo. Referencia: https://docs.m5stack.com/en/arduino/m5cores3/sdcard
- La captura prolongada necesita persistencia o almacenamiento temporal, señal visible de grabación, un mecanismo de inicio y parada, control de fallos y recuperación. Una lectura de tres segundos en RAM no resuelve reuniones largas.
- El envío a la VPS debe incorporar autenticación y cifrado; no registrar credenciales, transcripciones ni audio en Git. La ruta y los permisos de OpenClaw deben acordarse tras revisar su configuración.
- La documentación de OpenClaw distingue la autenticación de ChatGPT/Codex de la API facturada. La disponibilidad de transcripción depende del método de acceso y de la cuenta: https://docs.openclaw.ai/providers/openai/authentication

## Validación necesaria

Preparar la cadena de compilación indicada por el repositorio oficial, compilar y revisar particiones. Con el dispositivo conectado, probar captura y detención, calidad de audio, almacenamiento y recuperación ante fallos, transferencia autenticada, transcripción completa, generación de acta y entrega explícita. Verificar que continúen funcionando los componentes originales pertinentes y conservar un modo documentado de restaurar el firmware oficial.
