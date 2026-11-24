"""Kichik displey buferi (e-ink / OLED ko'rinishi)."""


class EinkDisplay:
    """Matn qatorlaridan tashkil topgan oddiy ekran buferi."""

    def __init__(self, columns: int = 24, rows: int = 5):
        self.columns = columns
        self.rows = rows
        self.lines = [""] * rows

    def clear(self) -> None:
        self.lines = [" " * self.columns for _ in range(self.rows)]

    def show(self, lines) -> None:
        """Ekranga qatorlar yozish (ortiqchasi kesiladi, kamini bo'sh qoldiradi)."""
        cleared = [" " * self.columns for _ in range(self.rows)]
        for index, line in enumerate(list(lines)[: self.rows]):
            cleared[index] = str(line)[: self.columns].ljust(self.columns)
        self.lines = cleared

    def render_ascii(self) -> str:
        """Displeyni terminalda chizish."""
        top = "┌" + "─" * (self.columns + 2) + "┐"
        bottom = "└" + "─" * (self.columns + 2) + "┘"
        body = "\n".join("│ " + line + " │" for line in self.lines)
        return "\n".join([top, body, bottom])
