# liblognorm Python bindings

[**liblognorm**](https://www.liblognorm.com/) is a high-performance log normalization library capable of real-time parsing and field extraction. It converts unstructured log messages into structured formats such as JSON, enabling consistent and efficient downstream processing.

This project provides modern, Pythonic bindings for **liblognorm 2.x**, implemented as a C extension module. It allows Python applications to perform normalization using the same optimized engine found in rsyslog and other production systems.

---

## Installation

### Requirements

*   **Python 3.5+**
*   **liblognorm 2.x** and its development headers
*   **pkg-config** utility
*   A C compiler toolchain (e.g., GCC or Clang)

On Debian-based systems (like Ubuntu), you can install these with:

```bash
sudo apt-get update
sudo apt-get install liblognorm-dev pkg-config python3-dev build-essential
```

### From Source

To build and install the package directly from this source repository:

```bash
git clone https://github.com/zaferb/python-liblognorm.git
cd python-liblognorm
pip install .
```

The setup script automatically uses `pkg-config` to discover the necessary compiler and linker flags for `liblognorm`.

---

## Usage Example

The following example demonstrates how to load a rulebase and normalize log lines entered via standard input.

```python
import liblognorm
import json
import sys

# Get the version of the linked liblognorm C library
print(f"Using liblognorm version: {liblognorm.version()}")

try:
    # 1. Initialize the Lognorm context
    ln = liblognorm.Lognorm()

    # 2. Load normalization rules from a file or directory
    ln.load("path/to/your/rules")

except liblognorm.Error as e:
    print(f"Initialization failed: {e}", file=sys.stderr)
    sys.exit(1)

print("\n>>> Ready. Enter log lines (Ctrl-D to exit):", file=sys.stderr)

# 3. Process lines from stdin
for line in sys.stdin:
    line = line.strip()
    if not line:
        continue

    try:
        # 4. Normalize the log line into a dictionary
        event = ln.normalize(line)
        print(json.dumps(event, indent=2))
    except liblognorm.Error as e:
        # Handle any normalization errors
        print(f"Error: {e}", file=sys.stderr)
```

---

## Error Handling

The library uses Pythonic exceptions for error handling. All exceptions inherit from the base `liblognorm.Error`. This allows you to catch errors with fine-grained control.

```python
try:
    event = ln.normalize(log_line)
except liblognorm.ParserError:
    print("Log line did not match any known format.")
except liblognorm.ConfigError:
    print("The loaded rulebase is invalid.")
except liblognorm.Error as e:
    print(f"A general liblognorm error occurred: {e}")
```

| Exception | Description |
| :--- | :--- |
| `liblognorm.MemoryError` | Out of memory during an operation. |
| `liblognorm.ConfigError` | Invalid rulebase configuration. |
| `liblognorm.ParserError` | Invalid parser state or no matching parser for a message. |
| `liblognorm.RuleError` | A rule is malformed or exceeds size limits. |
| `liblognorm.Error` | A generic or unknown error from the underlying library. |

---

## Type Annotations

This package is fully type-hinted via a bundled `__init__.pyi` stub file. This provides an excellent developer experience with full support for static type checkers (like Mypy) and rich autocompletion in modern IDEs.

---

## Character Encoding

The library's C core and the Python wrapper are designed to work exclusively with **UTF-8**, which is the standard encoding for modern data interchange.

To ensure correct and secure operation, users must adhere to the following principles:

1.  **Input Must Be a Python `str` (Unicode):** The `normalize()` method and other functions expect a standard Python 3 `str` object, not `bytes`. The wrapper will automatically raise a `TypeError` if `bytes` are passed.

2.  **UTF-8 is the Assumed Source Encoding:** The underlying `liblognorm` C library operates natively on UTF-8 byte streams. The Python C API bridge converts Python `str` objects to UTF-8 before passing them to the C library.

3.  **The User is Responsible for Correct Decoding:** It is the application developer's responsibility to ensure that any data read from external sources (files, network sockets, etc.) is correctly decoded into a Python `str` object before being passed to this library.

    *   **Example: Reading a UTF-8 file (Correct):**
        ```python
        with open("logfile.log", "r", encoding="utf-8") as f:
            for line in f:
                # 'line' is a valid Unicode string
                ln.normalize(line)
        ```

    *   **Example: Handling a UTF-16 file from a Windows source (Correct):**
        ```python
        with open("windows_log.log", "r", encoding="utf-16") as f:
            for line in f:
                # 'line' is correctly decoded into a Unicode string
                ln.normalize(line)
        ```

4.  **Other Encodings Are Not Supported:** Passing strings that contain characters from other encodings (like Latin-1 or CP-1252) which were not properly decoded may result in a `UnicodeEncodeError` from the wrapper or incorrect parsing behavior from the C library. The library will not attempt to guess the encoding or silently fix malformed data.

Following these guidelines ensures maximum performance, avoids silent data corruption, and makes your application's behavior predictable and robust.

---

## Attribution and License

This binding was originally developed by **Stanislaw Klekot** ([dozzie@jarowit.net](mailto:dozzie@jarowit.net)) for **Korbank S.A.** The original project can be found at [github.com/korbank/python-liblognorm](https://github.com/korbank/python-liblognorm).

This fork is maintained and modernized by **Zafer Balkan**, and includes the following improvements:

*   Modernized build system using `pyproject.toml` and `pkg-config`.
*   A fully Pythonic API with a robust exception hierarchy.
*   Complete type hints (`.pyi`) for static analysis and IDE support.
*   Improved memory safety and API consistency.
*   Python 3.5+ compatibility.

`python-liblognorm` is distributed under the **3-clause BSD license**. See the included `COPYING` file for the full text.

