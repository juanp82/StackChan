# Procesamiento local de reuniones — trabajo en curso

Este programa procesa un archivo de audio que ya exista. La captura desde StackChan y su transferencia todavía no están implementadas. No constituye un firmware instalable.

Requisitos: Python, `faster-whisper`, FFmpeg con `libmp3lame` y Ollama con el modelo elegido ya descargado. El primer uso de un modelo de Whisper puede descargarlo de internet; las grabaciones se procesan localmente. El tiempo de procesamiento y la precisión dependen del equipo, del modelo y del audio.

```powershell
python -m pip install faster-whisper
ollama pull qwen2.5:7b
python meeting_assistant/process.py grabacion.wav --output resultado-reunion
```

La carpeta de resultados debe ser nueva; contiene el MP3, la transcripción y el acta. Conservar el audio original hasta verificar los resultados. No se envía ningún correo por defecto. Para enviar el acta explícitamente, configurar `STACKCHAN_SMTP_HOST`, `STACKCHAN_SMTP_PORT` (por defecto 465), `STACKCHAN_SMTP_FROM` y `STACKCHAN_SMTP_USER`; añadir `--send-to destinatario@ejemplo.com`. Se solicitará la contraseña de aplicación sin guardarla en el repositorio. El servidor debe admitir SMTP con TLS implícito. La disponibilidad de SMTP depende del proveedor.

Las transcripciones y actas pueden contener errores; deben revisarse. Obtén el consentimiento y sigue las reglas aplicables antes de grabar. Los archivos producidos contienen información de la reunión: guárdalos en una carpeta protegida.
