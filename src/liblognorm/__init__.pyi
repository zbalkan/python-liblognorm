from typing import Any, Dict, Optional

# --- Module-Level Functions ---


def version() -> str:
    """
    Returns the version string of the underlying liblognorm C library.
    """
    ...


# --- Exception Classes ---

class Error(Exception):
    """Base exception for all liblognorm errors."""
    ...


class MemoryError(Error):
    """Raised when a memory allocation fails within the C library."""
    ...


class ConfigError(Error):
    """Raised on invalid rulebase configuration or syntax errors."""
    ...


class ParserError(Error):
    """
    Raised on an invalid internal parser state or when a log message
    does not match any known parser.
    """
    ...


class RuleError(Error):
    """
    Raised when a rule is malformed or exceeds internal size limits.
    """
    ...


# --- Main Class ---

class Lognorm:
    """
    Represents a liblognorm context, holding the loaded rulebase and state.
    """

    def __init__(self) -> None:
        """
        Initializes a new, empty liblognorm context.
        Raises MemoryError on failure.
        """
        ...

    def load(self, path: str) -> None:
        """
        Loads normalization rules from a file or a directory of files.

        If a directory is provided, all regular files within it are loaded.
        Rules are added to the existing context.

        Args:
            path: The file or directory path to the rulebase.

        Raises:
            FileNotFoundError: If the path does not exist.
            OSError: If a directory cannot be opened.
            ConfigError: If a rulebase has syntax errors.
            RuntimeError: If loading a rulebase file fails for other reasons.
        """
        ...

    def load_from_string(self, rules: str) -> None:
        """
        Loads normalization rules directly from a string.

        This is especially useful for testing or for applications where rules
        are generated dynamically.

        Args:
            rules: A string containing the rulebase content.

        Raises:
            ConfigError: If the rulebase content is invalid.
        """
        ...

    def normalize(self, log: str, strip: bool = True) -> Optional[Dict[str, Any]]:
        """
        Normalizes a log message string against the loaded rulebase.

        This is the primary function for processing logs. On success, it returns
        a dictionary representing the structured event. If the log message does
        not match any rule, it returns None.

        Args:
            log: The unstructured log message string to normalize.
            strip: If True (default), trailing whitespace and newlines are
                   removed from the log string before processing.

        Returns:
            A dictionary containing the normalized event fields, or None if
            the message did not match any rule.

        Raises:
            ParserError: If the message is invalid or causes a parser error.
            RuleError: If a rule-related limit is exceeded.
            MemoryError: If memory allocation fails during normalization.
            Error: For other generic processing errors.
        """
        ...
