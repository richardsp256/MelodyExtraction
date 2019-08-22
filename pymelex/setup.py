from setuptools import setup, find_packages

setup(name='pymelex',
      version='0.0.1',
      description='Python bindings for the MelodyExtraction library',
      packages=find_packages(),
      url='https://github.com/richardsp256/MelodyExtraction',
      setup_requires=['numpy'],
      install_requires = ['numpy']
     )
