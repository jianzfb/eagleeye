# encoding=utf-8
# @Time    : 17-7-25
# @File    : main.py
# @Author  : jian<jian@mltalker.com>

from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
from jinja2 import Environment, FileSystemLoader
import sys
import os
from . import flags
from . import help
import zipfile
import shutil

flags.DEFINE_string('project', None, 'project name')
flags.DEFINE_string("version", None, "project version")
flags.DEFINE_string("signature",None, "projct signature")
flags.DEFINE_string("eagleeye", "", "eagleeye path")
flags.DEFINE_string("opencv", "", "opencv path")
flags.DEFINE_string("abi", "arm64-v8a", "abi")
flags.DEFINE_string("build_type","Release","set build type")
flags.DEFINE_string("api_level","android-23", "android api level")
flags.DEFINE_string("name", "", "node name")
flags.DEFINE_string("inputport", "", "input port list")
flags.DEFINE_string("outputport","", "output port list")
flags.DEFINE_string("host_platform", "MACOS", "host platform")


FLAGS = flags.AntFLAGS
def main():
    # 模板引擎
    template_file_folder = os.path.join(os.path.dirname(__file__), 'templates')
    file_loader = FileSystemLoader(template_file_folder)
    env = Environment(loader=file_loader)

    if len(sys.argv) < 2 or sys.argv[1] == 'help':
      help.printhelp()
      return

    if(sys.argv[1] == 'node'):
      flags.cli_param_flags(sys.argv[2:])
      
      # 生成节点模板
      node_name = FLAGS.name()
      # 生成node.h
      template = env.get_template('project_node_h.template')
      inputport_list_str = FLAGS.inputport()
      inputport_list = inputport_list_str.split(',')

      outputport_list_str = FLAGS.outputport()
      outputport_list = outputport_list_str.split(',')

      output = template.render(MYNODE=node_name,
                                inputport=inputport_list,
                                outputport=outputport_list)
      # if not os.path.exists(os.path.join(os.curdir, "%s_plugin"%project_name)):
      #     os.mkdir(os.path.join(os.curdir, "%s_plugin"%project_name))
      #     return
      
      with open(os.path.join(os.curdir, "%s.h"%node_name),'w') as fp:
        fp.write(output)

      # 生成node.cpp
      template = env.get_template('project_node_cpp.template')
      output = template.render(MYNODE=node_name,
                                inputport=inputport_list,
                                outputport=outputport_list)

      with open(os.path.join(os.curdir, "%s.cpp"%node_name),'w') as fp:
        fp.write(output)

      return
    elif sys.argv[1] == 'release':
        flags.cli_param_flags(sys.argv[2:])
        if not os.path.exists(os.path.join(os.curdir, "package")):
          print("Couldnt find package folder")
          return

        project_name = FLAGS.project()
        if project_name is None or project_name == "":
          print("Scan package folder.")
          if os.path.exists(os.path.join(os.curdir, "package")):
            for f in os.listdir(os.path.join(os.curdir, "package")):
              if f[0] == '.':
                continue

              if os.path.isdir(os.path.join(os.curdir, "package", f)):
                project_name = f
                print("Finding default project %s"%project_name)
                break
        
        if project_name == "" or not os.path.exists(os.path.join(os.curdir, "package", project_name)):
          print("Package %s dont exist."%project_name)
          return

        package_folder = os.path.join(os.curdir, "package")
        # zip package
        z = zipfile.ZipFile("./package/%s.zip"%project_name, mode='w', compression=zipfile.ZIP_DEFLATED)
        for dirpath, dirnames, filenames in os.walk(package_folder):
          fpath = dirpath.replace(package_folder,'') 
          fpath = fpath and fpath + os.sep or ''
          for filename in filenames:
            z.write(os.path.join(dirpath, filename),fpath+filename)

        z.close()
        print("Success to build package.")
        return
    elif sys.argv[1] == "project":
      flags.cli_param_flags(sys.argv[2:])
      if FLAGS.eagleeye() == "":
        print("Must set eagleeye path.")
        return

      if FLAGS.project() is None:
        print("Must set project name.")
        return

      # 生成插件模板
      project_name = FLAGS.project()
      print("Generate project %s"%project_name)

      project_version = FLAGS.version()
      project_signature = FLAGS.signature()

      # 生成插件头文件
      template = env.get_template('project_plugin_header.template')
      output = template.render(project=project_name)

      if not os.path.exists(os.path.join(os.curdir, "%s_plugin"%project_name)):
          os.mkdir(os.path.join(os.curdir, "%s_plugin"%project_name))
      
      with open(os.path.join(os.curdir, "%s_plugin"%project_name, '%s_plugin.h'%project_name),'w') as fp:
        fp.write(output)

      # 生成插件源文件
      template = env.get_template('project_plugin_source.template')
      output = template.render(project=project_name,
                              version=project_version,
                              signature=project_signature)

      if not os.path.exists(os.path.join(os.curdir, "%s_plugin"%project_name)):
          os.mkdir(os.path.join(os.curdir, "%s_plugin"%project_name))
      
      with open(os.path.join(os.curdir, "%s_plugin"%project_name, '%s_plugin.cpp'%project_name),'w') as fp:
        fp.write(output)

      # 生成CMakeList.txt
      template = env.get_template('project_plugin_cmake.template')
      output = template.render(project=project_name,
                              eagleeye=FLAGS.eagleeye(),
                              abi=FLAGS.abi(),
                              opencv=FLAGS.opencv())

      if not os.path.exists(os.path.join(os.curdir, "%s_plugin"%project_name)):
          os.mkdir(os.path.join(os.curdir, "%s_plugin"%project_name))
      
      with open(os.path.join(os.curdir, "%s_plugin"%project_name, "CMakeLists.txt"),'w') as fp:
        fp.write(output)

      # 拷贝头文件
      shutil.copyfile(os.path.join(FLAGS.eagleeye(), "include", "eagleeye", "common", "EagleeyeModule.h"),
                      os.path.join("%s_plugin"%project_name, "EagleeyeModule.h"))

      # 生成build.sh
      template = env.get_template('project_shell.template')
      output = template.render(abi=FLAGS.abi(),
                              build_type=FLAGS.build_type(),
                              api_level=FLAGS.api_level(),
                              project=project_name)

      if not os.path.exists(os.path.join(os.curdir, "%s_plugin"%project_name)):
          os.mkdir(os.path.join(os.curdir, "%s_plugin"%project_name))
      
      with open(os.path.join(os.curdir, "%s_plugin"%project_name, "build.sh"),'w') as fp:
        fp.write(output)

      # 生成demo.cpp
      template = env.get_template('project_demo.template')
      output = template.render(project=project_name)

      if not os.path.exists(os.path.join(os.curdir, "%s_plugin"%project_name)):
          os.mkdir(os.path.join(os.curdir, "%s_plugin"%project_name))
      
      with open(os.path.join(os.curdir, "%s_plugin"%project_name, '%s_demo.cpp'%project_name),'w') as fp:
        fp.write(output)

      # 生成VS CODE 配置文件
      if not os.path.exists(os.path.join(os.curdir,  "%s_plugin"%project_name, ".vscode")):
        os.mkdir(os.path.join(os.curdir, "%s_plugin"%project_name, ".vscode"))

      # VS CODE - settings.json
      template = env.get_template('project_VS_settings_json.template')
      output = template.render(abi=FLAGS.abi(),
                              build_type=FLAGS.build_type(),
                              api_level=FLAGS.api_level())
      with open(os.path.join(os.curdir, "%s_plugin"%project_name, ".vscode", "settings.json"), 'w') as fp:
        fp.write(output)

      # VS CODE - c_cpp_properties.json
      template = env.get_template('project_VS_c_cpp_properties.template')
      output = template.render(eagleeye=FLAGS.eagleeye())
      with open(os.path.join(os.curdir, "%s_plugin"%project_name, ".vscode", "c_cpp_properties.json"), 'w') as fp:
        fp.write(output)

      # VS CODE - tasks.json
      template = env.get_template('project_VS_tasks_json.template')
      output = template.render(project=project_name)
      with open(os.path.join(os.curdir, "%s_plugin"%project_name, ".vscode", "tasks.json"), 'w') as fp:
        fp.write(output)

      # VS CODE - cmake-kits.json
      template = env.get_template('project_VS_cmake-kits_json.template')
      output = template.render(host_platform=FLAGS.host_platform())
      with open(os.path.join(os.curdir, "%s_plugin"%project_name, ".vscode", "cmake-kits.json"), 'w') as fp:
        fp.write(output)

      # VS CODE - run.sh
      template = env.get_template('project_run.template')
      output = template.render(project=project_name,
                                eagleeye=FLAGS.eagleeye(),
                                abi=FLAGS.abi())

      with open(os.path.join(os.curdir, "%s_plugin"%project_name, "run.sh"), 'w') as fp:
        fp.write(output)

      # 生成说明文档

      # 生成插件包目录
      os.makedirs(os.path.join(os.curdir,  "%s_plugin"%project_name, "package", project_name, "resource"))
      template = env.get_template('project_plugin_config.template')
      output = template.render(project=project_name)
      with open(os.path.join(os.curdir,  "%s_plugin"%project_name, "package", project_name, "resource", "config.json"), 'w') as fp:
        fp.write(output)


if __name__ == '__main__':
  main()