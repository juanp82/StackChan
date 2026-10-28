import unittest
from unittest.mock import patch, MagicMock
import process

class ProcessingTests(unittest.TestCase):
    def test_chunks_preserve_all_text(self):
        text = 'uno\ndos\n' + 'x' * 15 + '\nfin\n'
        self.assertEqual(''.join(process.chunks(text, 5)), text)

    def test_summary_of_empty_transcript_fails(self):
        with self.assertRaises(ValueError):
            process.summarize('', 'example')

    def test_summary_combines_multiple_parts(self):
        with patch.object(process, 'run_command', side_effect=['parte uno', 'parte dos', 'acta final']) as run:
            self.assertEqual(process.summarize('a' * 12000 + '\n' + 'b', 'model'), 'acta final')
            self.assertEqual(run.call_count, 3)
            self.assertIn('parte uno', run.call_args.kwargs['input_text'])

    def test_email_requires_explicit_configuration(self):
        with patch.dict('os.environ', {}, clear=True):
            with self.assertRaises(ValueError):
                process.email_summary('a@example.com', 'Acta', 'Texto')

    def test_email_uses_tls_and_sends_only_acta(self):
        settings = {'STACKCHAN_SMTP_HOST': 'smtp.example.com', 'STACKCHAN_SMTP_USER': 'user',
                    'STACKCHAN_SMTP_FROM': 'user@example.com'}
        with patch.dict('os.environ', settings, clear=True), patch('process.getpass.getpass', return_value='secret'), patch('process.smtplib.SMTP_SSL') as smtp_cls:
            smtp = smtp_cls.return_value.__enter__.return_value
            process.email_summary('target@example.com', 'Acta', 'Resumen')
            smtp_cls.assert_called_once_with('smtp.example.com', 465, timeout=30)
            smtp.login.assert_called_once_with('user', 'secret')
            msg = smtp.send_message.call_args.args[0]
            self.assertEqual(msg['To'], 'target@example.com')
            self.assertEqual(msg.get_content().strip(), 'Resumen')

if __name__ == '__main__':
    unittest.main()
