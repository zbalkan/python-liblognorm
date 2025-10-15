#!/usr/bin/env python3
from setuptools import setup, Extension
import subprocess
import shlex


def pkg_config_flags(package: str, flag: str) -> list[str]:
    """Retrieve compiler/linker flags via pkg-config if available."""
    try:
        output = subprocess.check_output(
            ["pkg-config", flag, package],
            stderr=subprocess.DEVNULL
        ).decode().strip()
        return shlex.split(output)
    except (subprocess.CalledProcessError, FileNotFoundError):
        return []


cflags = pkg_config_flags("lognorm", "--cflags")
ldflags = pkg_config_flags("lognorm", "--libs")

ext = Extension(
    "liblognorm",
    sources=["c_lib/lognorm.c"],
    extra_compile_args=cflags,
    extra_link_args=ldflags,
)

setup(
    name="liblognorm",
    version="0.2.0",
    description="Python bindings for liblognorm — updated fork",
    long_description=(
        "This is a maintained fork of python-liblognorm originally written by "
        "Stanislaw Klekot for Korbank S.A. (http://korbank.com/). "
        "The primary distribution point of the original project is "
        "https://github.com/korbank/python-liblognorm.\n\n"
        "This fork, maintained by Zafer Balkan, adds Python 3.5+ support, "
        "dropped, Python 2 support, improved error handling,"
        " and added type annotations. "
        "python-liblognorm is distributed under the 3-clause BSD license; "
        "see the COPYING file for details."
    ),
    author="Stanislaw Klekot (original), Zafer Balkan (maintainer)",
    author_email="",
    maintainer="Zafer Balkan",
    maintainer_email="",
    url="https://github.com/zaferbalkan/python-liblognorm",
    license="BSD-3-Clause",
    python_requires=">=3.5",
    ext_modules=[ext],
    package_data={
        "": ["liblognorm.pyi", "py.typed"],
    },
    include_package_data=True,
    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: C",
        "License :: OSI Approved :: BSD License",
        "Operating System :: POSIX :: Linux",
        "Topic :: System :: Logging",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "Development Status :: 5 - Production/Stable",
    ],
)
