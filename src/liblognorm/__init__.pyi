from typing import Any, Dict, Optional


def version() -> str: ...


# Error classes
class Error(Exception):
    ...


class MemoryError(Error):
    ...


class ConfigError(Error):
    ...


class ParserError(Error):
    ...


class RuleError(Error):
    ...


class Lognorm:
    """liblognorm context"""

    def __init__(self) -> None: ...
    def load(self, path: str) -> None: ...
    def normalize(self, log: str, strip: bool = False) -> Optional[Dict[str, Any]]: ...
    def get_error(self) -> Optional[str]: ...
