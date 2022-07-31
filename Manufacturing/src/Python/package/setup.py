# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import codecs
import os.path
import pathlib

from setuptools import find_packages, setup


def read(rel_path):
    here = os.path.abspath(os.path.dirname(__file__))
    with codecs.open(os.path.join(here, rel_path), 'r') as fp:
        return fp.read()


def get_version(rel_path):
    for line in read(rel_path).splitlines():
        if line.startswith('__version__'):
            delim = '"' if '"' in line else "'"
            return line.split(delim)[1]
    else:
        raise RuntimeError("Unable to find version string.")


here = pathlib.Path(__file__).parent.resolve()

# Get the long description from the README file
long_description = (here / "README.md").read_text(encoding="utf-8")

DEPENDENCIES = [
    "requests >= 2.27.1",
    "requests-toolbelt >= 0.9.1",
]

setup(
    name='microsoft-azure-sphere-deviceapi',
    version=get_version(
        "microsoft_azure_sphere_deviceapi/__init__.py"),
    packages=find_packages(exclude=["*test*", "tests*"]),
    license='MIT',
    description='A library for interacting with Azure Sphere using the inbuilt REST server.',
    long_description=long_description,
    long_description_content_type="text/markdown",
    author="Microsoft Corporation",
    author_email="azspheremfrsamplesup@microsoft.com",
    url='https://github.com/Azure/azure-sphere-samples/Manufacturing/src/Python/',
    project_urls={
        "Bug Reports": "https://github.com/Azure/azure-sphere-samples/issues",
        "Source": "https://github.com/Azure/azure-sphere-samples/Manufacturing/src/Python/",
    },
    python_requires=">=3.8",
    classifiers=["Programming Language :: Python :: 3",
                 "Operating System :: OS Independent"],
    install_requires=DEPENDENCIES,
    include_package_data=True,
    package_data={  # Optional
        "microsoft_azure_sphere_deviceapi": ["certs/*.pem"],
    },)
