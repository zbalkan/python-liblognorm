"""Python bindings for liblognorm."""

from importlib import import_module as _import_module

# Load the C extension (built as liblognorm._liblognorm)
from ._liblognorm import (ConfigError, Error, Lognorm, MemoryError,
                          ParserError, RuleError)

__all__ = [
    "Lognorm",
    "Error",
    "MemoryError",
    "ConfigError",
    "ParserError",
    "RuleError",
]
