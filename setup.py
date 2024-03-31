import setuptools

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="real_pymate",
    version="1.0.1",
    url="https://github.com/akcarsten/pyMate",
    author="Carsten Klein",
    description="Framework to process primate fMRI and electrophysiological data",
    long_description=long_description,
    long_description_content_type="text/markdown",
    packages=setuptools.find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
)