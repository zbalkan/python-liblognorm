# liblognorm Python bindings

[**liblognorm**](https://www.liblognorm.com/) is a high-performance log normalization library capable of real-time parsing and field extraction. It converts unstructured log messages into structured formats such as JSON or [CEE](https://cee.mitre.org/), enabling consistent and efficient downstream processing.

This project provides **Python bindings for liblognorm 2.x**, implemented as a C extension module.
It allows Python applications to perform normalization using the same optimized engine used in rsyslog and other production systems.

---

## Installation

### Requirements

* **Python 3.5+**
* **liblognorm 2.x** and its development headers
* **pkg-config** utility
* A C compiler toolchain (e.g., GCC or Clang)

On Debian/Ubuntu systems:

```bash
sudo apt install liblognorm-dev pkg-config python3-dev build-essential
```

### Build and install

To build directly from source:

```bash
git clone https://github.com/zaferb/python-liblognorm.git
cd python-liblognorm
python3 -m pip install .
```

To build a wheel for distribution:

```bash
python3 -m build
```

The setup script automatically discovers compiler and linker flags using `pkg-config`.

---

## Usage Example

```python
import liblognorm
import json
import sys

print("liblognorm version:", liblognorm.Lognorm.version())  # C library version

ln = liblognorm.Lognorm(rules="parsing.rules")  # expects a rulebase file

for line in sys.stdin:
    try:
        event = ln.normalize(line, strip=True)
        if event is not None:
            print(json.dumps(event))
    except liblognorm.ConfigError as e:
        print("Rulebase configuration error:", e, file=sys.stderr)
    except liblognorm.ParserError as e:
        print("Parsing error:", e, file=sys.stderr)
    except liblognorm.Error as e:
        print("Unhandled liblognorm error:", e, file=sys.stderr)
```

---

## Error Handling

All errors are raised as subclasses of `liblognorm.Error`:

| Exception     | Description                              |
| ------------- | ---------------------------------------- |
| `MemoryError` | Out of memory                            |
| `ConfigError` | Invalid rulebase configuration           |
| `ParserError` | Invalid parser state or message mismatch |
| `RuleError`   | Rulebase too large or malformed          |
| `Error`       | Generic or unknown error                 |

`normalize()` returns a Python `dict` on success or raises one of the above exceptions on failure.

---

## Logging and Debug Output

The extension integrates with Python’s standard [`logging`](https://docs.python.org/3/library/logging.html) module.

If the logger named **`liblognorm`** has its level set to `DEBUG` **when a `Lognorm` instance is created**, internal debug output from the underlying C library will be enabled automatically.

Example:

```python
import logging, liblognorm

logging.basicConfig(level=logging.INFO)
ln1 = liblognorm.Lognorm(rules="rules.rb")  # debug disabled

logging.getLogger("liblognorm").setLevel(logging.DEBUG)
ln2 = liblognorm.Lognorm(rules="rules.rb")  # debug enabled
```

Changing the Python logging configuration **after** initialization does not retroactively toggle debug output.

---

## Type Annotations

Static type definitions are provided in `liblognorm.pyi`, enabling full support for type checkers and IDE autocompletion.

---

## Attribution and License

This binding was originally developed by **Stanislaw Klekot** ([dozzie@jarowit.net](mailto:dozzie@jarowit.net)) for **Korbank S.A.** ([https://korbank.com/](https://korbank.com/)).
Primary distribution point: [https://github.com/korbank/python-liblognorm](https://github.com/korbank/python-liblognorm)

This fork is maintained and modernized by **Zafer Balkan**, with:

* Updated build system using `pkg-config`
* Extended exception hierarchy and Python-native error handling
* Integration with Python’s standard logging module
* Type hints (`.pyi`) for static analysis
* Improved memory safety and Pythonic API consistency

`python-liblognorm` is distributed under the **3-clause BSD license**.
See the included `COPYING` file for full text.