#!/usr/bin/env python3
# cython: boundscheck=False


from setuptools import setup, Extension
from Cython.Build import cythonize

extensions = [
    Extension("coffee_grinder", ["coffee_grinder.pyx"])
]

setup(
    name="coffee_grinder",
    version="1.0",
    ext_modules = cythonize(extensions)
)
