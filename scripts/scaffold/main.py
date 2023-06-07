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
import json

flags.DEFINE_string('project', None, 'project name')
flags.DEFINE_string("version", None, "project version")
flags.DEFINE_string("signature",None, "projct signature")
flags.DEFINE_string("eagleeye", "", "eagleeye path")
flags.DEFINE_string("opencv", "", "opencv path")
flags.DEFINE_string("abi", "arm64-v8a,armeabi-v7a,X86-64,X86", "abi")
flags.DEFINE_string("build_type","Release","set build type")
flags.DEFINE_string("api_level","android-23", "android api level")
flags.DEFINE_string("inputport", "", "input port list")
flags.DEFINE_string("outputport","", "output port list")
flags.DEFINE_string("host_platform", "MACOS", "host platform")
flags.DEFINE_string("structure", None, "plugin load from json")
flags.DEFINE_string("paddlelite", "", "paddle lib")
flags.DEFINE_string("tnn", "", "tnn lib")

FLAGS = flags.AntFLAGS
def main():
    # 模板引擎
    template_file_folder = os.path.join(os.path.dirname(__file__), 'templates')
    file_loader = FileSystemLoader(template_file_folder)
    env = Environment(loader=file_loader)

    if len(sys.argv) < 2 or sys.argv[1] == 'help':
      help.printhelp()
      sys.exit(-1)

    if(sys.argv[1].startswith('node')):
      flags.cli_param_flags(sys.argv[2:])

      node_name = FLAGS.project()
      if node_name is None:
        print("Must set node project name.")
        sys.exit(-1)

      project_folder = os.curdir
      if sys.argv[1] == "node/project":
        if not os.path.exists(os.path.join(os.curdir, "%s_project"%node_name)):
          os.mkdir(os.path.join(os.curdir, "%s_project"%node_name))
          project_folder = os.path.join(os.curdir, "%s_project"%node_name)
      
      # 生成节点模板
      # 生成node.h
      template = env.get_template('project_node_h.template')
      inputport_list_str = FLAGS.inputport()
      inputport_list = inputport_list_str.split(',')

      outputport_list_str = FLAGS.outputport()
      outputport_list = outputport_list_str.split(',')

      output = template.render(MYNODE=node_name,
                                inputport=inputport_list,
                                outputport=outputport_list)
      with open(os.path.join(project_folder, "%s.h"%node_name),'w') as fp:
        fp.write(output)

      # 生成node.cpp
      template = env.get_template('project_node_cpp.template')
      output = template.render(MYNODE=node_name,
                                inputport=inputport_list,
                                outputport=outputport_list)

      with open(os.path.join(project_folder, "%s.cpp"%node_name),'w') as fp:
        fp.write(output)

      if sys.argv[1] == "node/project":
        if FLAGS.eagleeye() == "":
          print("Must set eagleeye path.")
          sys.exit(-1)

        # 生成CMakeList.txt
        template = env.get_template('project_node_cmake.template')
        output = template.render(project=FLAGS.project(),
                                eagleeye=FLAGS.eagleeye(),
                                opencv=FLAGS.opencv(),
                                paddlelite=FLAGS.paddlelite())

        with open(os.path.join(project_folder, "CMakeLists.txt"),'w') as fp:
          fp.write(output)

        # 生成编译shell脚本
        template = env.get_template('project_node_shell.template')
        output = template.render(build_abis=FLAGS.abi().split(','),
                                build_type=FLAGS.build_type(),
                                api_level=FLAGS.api_level(),
                                project=node_name)

        with open(os.path.join(project_folder, "build.sh"),'w') as fp:
          fp.write(output)
      sys.exit(0)
    elif sys.argv[1] == 'release':
        flags.cli_param_flags(sys.argv[2:])
        if not os.path.exists(os.path.join(os.curdir, "package")):
          print("Couldnt find package folder")
          sys.exit(-1)

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
        
        for abi in FLAGS.abi().split(','):
          package_folder = os.path.join(os.curdir, "package", abi)
          if not os.path.exists(os.path.join(package_folder, project_name, 'lib%s.so'%project_name)):
            print("Package %s/%s dont exist."%(project_name, abi))
            sys.exit(-1)
          
          if not os.path.exists(os.path.join(package_folder, project_name, 'resource')):
            os.makedirs(os.path.join(package_folder, project_name, 'resource'))

          if not os.path.exists(os.path.join(package_folder, project_name, 'resource', 'config.json')):
            template = env.get_template('project_plugin_config.template')
            output = template.render(project=project_name)
            with open(os.path.join(package_folder, project_name, 'resource', "config.json"), 'w') as fp:
              fp.write(output)

          # 对监控变量中的部分信息进行重新修正 (针对平台自动生成的信息)
          with open(os.path.join(package_folder, project_name, 'resource', "config.json"), 'r') as fp:
            config_content = json.load(fp)

            monitor_info = config_content['monitor']
            for k,v in monitor_info.items():
              if v['control'] == 'select':
                for v_value in v['value']:
                  abcde = v_value['image'].split('/')
                  if len(abcde) == 5:
                    # 自动生成的路径
                    monitor_node, _, _ , _ , file_name = abcde
                    v_value['image'] = "/".join(["images", monitor_node, file_name.split('-')[-1]])
                  
                  file_name = abcde[-1]
                  # 检查图片是否存在
                  if not os.path.exists(os.path.join(package_folder, project_name, 'resource', v_value['image'])):
                    if not os.path.exists(os.path.join(package_folder, project_name, 'resource', '/'.join(v_value['image'].split('/')[:-1]))):
                      os.makedirs(os.path.join(package_folder, project_name, 'resource', '/'.join(v_value['image'].split('/')[:-1])))

                    shutil.move(os.path.join(package_folder, project_name, 'resource', 'images', file_name), 
                                os.path.join(package_folder, project_name, 'resource', v_value['image']))

            config_content['monitor'] =  monitor_info
          with open(os.path.join(package_folder, project_name, 'resource', "config.json"), 'w') as fp:
            json.dump(config_content, fp)

          # zip package
          z = zipfile.ZipFile("./package/%s.%s.zip"%(project_name, abi), mode='w', compression=zipfile.ZIP_DEFLATED)
          for dirpath, dirnames, filenames in os.walk(package_folder):
            fpath = dirpath.replace(package_folder,'') 
            fpath = fpath and fpath + os.sep or ''
            for filename in filenames:
              z.write(os.path.join(dirpath, filename),fpath+filename)

          z.close()
        print("Success to build package.")
        sys.exit(0)
    elif sys.argv[1] == "project":
      flags.cli_param_flags(sys.argv[2:])
      if FLAGS.eagleeye() == "":
        print("Must set eagleeye path.")
        sys.exit(-1)

      if FLAGS.project() is None:
        print("Must set project name.")
        sys.exit(-1)

      # 生成插件模板
      project_name = FLAGS.project()
      print("Generate project %s."%project_name)

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
      PrecompiledModels = []
      if FLAGS.structure() is None:
        template = env.get_template('project_plugin_source.template')
        output = template.render(project=project_name,
                        version=project_version,
                        signature=project_signature)
      else:
        template = env.get_template('project_plugin_source_auto.template')

        with open(FLAGS.structure(), 'r') as fp:
          structure_json = json.load(fp)
        pipeline_info = structure_json['pipeline']
        nodes_attrs = pipeline_info['attribute']

        PaddleOpList = []
        TNNOpList = []
        for node_name, node_attr in nodes_attrs.items():
          if node_attr['type'] == "NNNode":
            nn_attr = node_attr['param']['attribute']

            for k,v in nn_attr.items():
              if v['type'] == 'PaddleOp':
                PaddleOpList.append({
                  'name': k,
                  'cls': v['type'],
                  'input': v['param']['input']['name'],
                  'output': v['param']['output']['name']
                })
              elif v['type'] == 'TNNOp':
                TNNOpList.append({
                  'name': k,
                  'cls': v['type'],
                  'input': v['param']['input']['name'],
                  'output': v['param']['output']['name']
                })
          elif 'precompiled' in node_attr:
            PrecompiledModels.append(node_attr['type'])

        output = template.render(project=project_name,
                        version=project_version,
                        signature=project_signature,
                        custom_paddle_ops=PaddleOpList,
                        custom_tnn_ops=TNNOpList,
                        precompiled_models=PrecompiledModels
                        )

      if not os.path.exists(os.path.join(os.curdir, "%s_plugin"%project_name)):
          os.mkdir(os.path.join(os.curdir, "%s_plugin"%project_name))
      
      with open(os.path.join(os.curdir, "%s_plugin"%project_name, '%s_plugin.cpp'%project_name),'w') as fp:
        fp.write(output)

      # 生成CMakeList.txt
      template = env.get_template('project_plugin_cmake.template')
      output = template.render(project=project_name,
                              eagleeye=FLAGS.eagleeye(),
                              opencv=FLAGS.opencv(),
                              paddlelite=FLAGS.paddlelite(),
                              tnn=FLAGS.tnn(),
                              precompiled_models=PrecompiledModels)

      if not os.path.exists(os.path.join(os.curdir, "%s_plugin"%project_name)):
          os.mkdir(os.path.join(os.curdir, "%s_plugin"%project_name))
      
      with open(os.path.join(os.curdir, "%s_plugin"%project_name, "CMakeLists.txt"),'w') as fp:
        fp.write(output)

      # 生成cmake文件夹
      os.makedirs(os.path.join(os.curdir, "%s_plugin"%project_name, 'cmake'))
      shutil.copy(os.path.join(os.path.dirname(__file__), 'cmake', 'ccache.cmake'), os.path.join(os.curdir, "%s_plugin"%project_name, 'cmake', 'ccache.cmake'))
      shutil.copy(os.path.join(os.path.dirname(__file__), 'cmake', 'FindTensorRT.cmake'), os.path.join(os.curdir, "%s_plugin"%project_name, 'cmake', 'FindTensorRT.cmake'))

      # 拷贝头文件
      shutil.copyfile(os.path.join(FLAGS.eagleeye(), "include", "eagleeye", "common", "EagleeyeModule.h"),
                      os.path.join("%s_plugin"%project_name, "EagleeyeModule.h"))

      # 生成build.sh
      template = env.get_template('project_shell.template')
      output = template.render(build_abis=FLAGS.abi().split(','),
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
      output = template.render(abi=FLAGS.abi().split(',')[0],
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
                                abi=FLAGS.abi().split(',')[0],
                                paddlelite=FLAGS.paddlelite())

      with open(os.path.join(os.curdir, "%s_plugin"%project_name, "run.sh"), 'w') as fp:
        fp.write(output)

      # 生成数据文件夹 data
      os.makedirs(os.path.join(os.curdir, 'data'), exist_ok=True)
      # 生成模型文件夹 model
      os.makedirs(os.path.join(os.curdir, 'model'), exist_ok=True)

      # 生成说明文档

      # 生成插件包目录
      # os.makedirs(os.path.join(os.curdir,  "%s_plugin"%project_name, "package", project_name, "resource"))
      # template = env.get_template('project_plugin_config.template')
      # output = template.render(project=project_name)
      # with open(os.path.join(os.curdir,  "%s_plugin"%project_name, "package", project_name, "resource", "config.json"), 'w') as fp:
      #   fp.write(output)


if __name__ == '__main__':
  main()