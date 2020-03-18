# encoding=utf-8
# @Time    : 17-7-25
# @File    : flags.py
# @Author  : jian<jian@mltalker.com>
from __future__ import division
from __future__ import unicode_literals
from __future__ import print_function
import sys
import getopt
_options = {}


class _FLAGS(object):
  def __init__(self):
    self._flags = []
    self._flags_name = []

  def add(self, flag):
    setattr(self, flag.name, lambda: flag.value)
    self._flags.append(flag)
    self._flags_name.append(flag.name)

  @property
  def shortopts(self):
    return ':'.join(self._flags_name)

  @property
  def longopts(self):
    a =  [f.name + "=" for f in self._flags if f.has_value]
    a.extend([f.name for f in self._flags if not f.has_value])
    return a

AntFLAGS = _FLAGS()


class VarFlag(object):
  def __init__(self, name, default, var_help, has_value=True):
    self._name = name
    self._var_help = var_help
    self._default = default
    self._has_value = has_value

  @property
  def name(self):
    return self._name
  @property
  def help(self):
    return self._var_help
  @property
  def default(self):
    return self._default
  @property
  def has_value(self):
    return self._has_value

  @property
  def value(self):
    raise NotImplementedError


class _FloatFlag(VarFlag):
  def __init__(self, name, default, var_help):
    super(_FloatFlag, self).__init__(name, default, var_help)

  @property
  def value(self):
    global _options
    if '--' + self.name in _options:
      return float(_options['--' + self.name])
    elif '-' + self.name in _options:
      return float(_options['--' + self.name])

    return float(self.default) if self.default is not None else None


def DEFINE_float(name, default, var_help):
  global AntFLAGS
  AntFLAGS.add(_FloatFlag(name, default, var_help))


class _IntegerFlag(VarFlag):
  def __init__(self, name, default, var_help):
    super(_IntegerFlag, self).__init__(name, default, var_help)

  @property
  def value(self):
    global _options
    if '--' + self.name in _options:
      return int(_options['--' + self.name])
    elif '-' + self.name in _options:
      return int(_options['--' + self.name])

    return int(self.default) if self.default is not None else None


def DEFINE_integer(name, default, var_help):
  global AntFLAGS
  AntFLAGS.add(_IntegerFlag(name, default, var_help))


class _StringFlag(VarFlag):
  def __init__(self, name, default, var_help):
    super(_StringFlag, self).__init__(name, default, var_help)

  @property
  def value(self):
    global _options
    if '--' + self.name in _options:
      return str(_options['--' + self.name])
    elif '-' + self.name in _options:
      return str(_options['--' + self.name])

    return str(self.default) if self.default is not None else None


def DEFINE_string(name, default, var_help):
  global AntFLAGS
  AntFLAGS.add(_StringFlag(name, default, var_help))


class _BooleanFlag(VarFlag):
  def __init__(self, name, default, var_help):
    super(_BooleanFlag, self).__init__(name, default, var_help)

  @property
  def value(self):
    global _options
    if '--' + self.name in _options:
      return bool(int(_options['--' + self.name]))
    elif '-' + self.name in _options:
      return bool(int(_options['--' + self.name]))

    return bool(self.default) if self.default is not None else None


def DEFINE_boolean(name, default, var_help):
  global AntFLAGS
  AntFLAGS.add(_BooleanFlag(name, default, var_help))


class _IndicatorFlag(VarFlag):
  def __init__(self, name, var_help):
    super(_IndicatorFlag, self).__init__(name, '', var_help, False)

  @property
  def value(self):
    global _options
    if '--' + self.name in _options:
      return True
    elif '-' + self.name in _options:
      return True

    return False

def DEFINE_indicator(name, var_help):
  global AntFLAGS
  AntFLAGS.add(_IndicatorFlag(name, var_help))


def cli_param_flags(cmd_str=None):
  global _options
  global AntFLAGS

  if cmd_str is None:
    cmd_str = sys.argv[2:]

  kv_args, args = getopt.getopt(cmd_str, AntFLAGS.shortopts, AntFLAGS.longopts)
  for k, v in kv_args:
    _options[k] = v

def clear_cli_param_flags():
  global _options
  _options = {}