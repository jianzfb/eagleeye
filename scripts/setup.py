from setuptools import setup

setup(name='eagleeye',
      version='0.0.1',
      description='eagleeye dataflow framework scaffold',
      __short_description__='eagleeye dataflow framework scaffold',
      url='https://github.com/jianzfb/eagleeye',
      author='jian',
      author_email='jian@mltalker.com',
      packages=['scaffold'],
      entry_points={'console_scripts': ['eagleeye-cli=scaffold.main:main'], },
      long_description="",
      include_package_data=True,
      zip_safe=False,)
