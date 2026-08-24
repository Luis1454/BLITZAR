import tempfile
import unittest
from pathlib import Path

from format_blocks import read_lines, write_lines


class FormatBlocksTests(unittest.TestCase):
    def test_detects_mixed_line_endings(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "Mixed.cpp"
            path.write_bytes(b"int first = 0;\r\nint second = 1;\n")

            lines, newline, trailing_newline, mixed = read_lines(path)

            self.assertEqual(lines, ["int first = 0;", "int second = 1;"])
            self.assertEqual(newline, "\r\n")
            self.assertTrue(trailing_newline)
            self.assertTrue(mixed)

    def test_write_lines_normalizes_mixed_input(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "Mixed.cpp"
            path.write_bytes(b"int first = 0;\r\nint second = 1;\n")
            lines, newline, trailing_newline, _ = read_lines(path)

            write_lines(path, lines, newline, trailing_newline)

            self.assertEqual(
                path.read_bytes(), b"int first = 0;\r\nint second = 1;\r\n"
            )


if __name__ == "__main__":
    unittest.main()
