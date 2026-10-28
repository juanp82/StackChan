"""Procesa una grabación local. Esta herramienta aún no está conectada al firmware."""
import argparse
import getpass
import os
from pathlib import Path
import shutil
import smtplib
import subprocess
import sys
from email.message import EmailMessage

PROMPT = ("Redacta un acta de reunión en español basada exclusivamente en la transcripción. "
          "Incluye resumen, acuerdos, tareas con responsables y fechas solo cuando se indiquen, "
          "y preguntas pendientes. Señala las incertidumbres y no inventes información.\n\n")


def run_command(command, *, input_text=None):
    return subprocess.run(command, input=input_text, text=True, check=True,
                          stdout=subprocess.PIPE).stdout


def chunks(text, size=12000):
    """Split at line boundaries; preserve every line, including unusually long lines."""
    current = []
    length = 0
    for line in text.splitlines(keepends=True):
        if current and length + len(line) > size:
            yield ''.join(current)
            current, length = [], 0
        current.append(line)
        length += len(line)
    if current:
        yield ''.join(current)


def summarize(text, model):
    parts = list(chunks(text))
    if not parts:
        raise ValueError('La transcripción está vacía; no se generó el resumen.')
    results = [run_command(['ollama', 'run', model], input_text=PROMPT + part).strip()
               for part in parts]
    if len(results) == 1:
        return results[0]
    return run_command(['ollama', 'run', model], input_text=(
        'Combina estas actas parciales en una sola acta en español. '
        'Conserva las incertidumbres y no inventes datos.\n\n' + '\n\n'.join(results))).strip()


def email_summary(recipient, subject, summary):
    host = os.environ.get('STACKCHAN_SMTP_HOST')
    sender = os.environ.get('STACKCHAN_SMTP_FROM')
    user = os.environ.get('STACKCHAN_SMTP_USER')
    if not all((host, sender, user)):
        raise ValueError('Configura STACKCHAN_SMTP_HOST, STACKCHAN_SMTP_FROM y STACKCHAN_SMTP_USER.')
    port = int(os.environ.get('STACKCHAN_SMTP_PORT', '465'))
    password = getpass.getpass('Contraseña de aplicación SMTP: ')
    msg = EmailMessage()
    msg['From'], msg['To'], msg['Subject'] = sender, recipient, subject
    msg.set_content(summary)
    with smtplib.SMTP_SSL(host, port, timeout=30) as smtp:
        smtp.login(user, password)
        smtp.send_message(msg)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('audio', type=Path, help='Grabación existente (WAV u otro formato compatible con ffmpeg)')
    parser.add_argument('--output', type=Path, required=True, help='Carpeta nueva para los resultados')
    parser.add_argument('--whisper-model', default='small', help='Modelo local de faster-whisper')
    parser.add_argument('--ollama-model', default='qwen2.5:7b', help='Modelo local de Ollama ya descargado')
    parser.add_argument('--send-to', help='Envía el acta por SMTP al destinatario indicado')
    args = parser.parse_args()
    if not args.audio.is_file():
        parser.error('No existe el archivo de audio.')
    if not shutil.which('ffmpeg') or not shutil.which('ollama'):
        parser.error('Instala ffmpeg y Ollama y agrégalos al PATH.')
    try:
        from faster_whisper import WhisperModel
    except ImportError:
        parser.error('Instala faster-whisper en el entorno de Python.')
    args.output.mkdir(parents=True, exist_ok=False)
    mp3 = args.output / 'reunion.mp3'
    run_command(['ffmpeg', '-nostdin', '-v', 'error', '-i', str(args.audio),
                 '-vn', '-codec:a', 'libmp3lame', '-b:a', '64k', str(mp3)])
    model = WhisperModel(args.whisper_model, device='cpu', compute_type='int8')
    segments, _ = model.transcribe(str(mp3), language='es', vad_filter=True)
    transcript = ''.join(f'[{int(s.start // 60):02d}:{int(s.start % 60):02d}] {s.text.strip()}\n'
                         for s in segments)
    (args.output / 'transcripcion.txt').write_text(transcript, encoding='utf-8')
    summary = summarize(transcript, args.ollama_model)
    if not summary:
        raise ValueError('El modelo devolvió un resumen vacío.')
    (args.output / 'acta.txt').write_text(summary + '\n', encoding='utf-8')
    if args.send_to:
        email_summary(args.send_to, 'Acta de reunión', summary)
    print(f'Resultados: {args.output.resolve()}')


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, subprocess.CalledProcessError, smtplib.SMTPException) as exc:
        print(f'Error: {exc}', file=sys.stderr)
        sys.exit(1)
