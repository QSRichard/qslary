#include "config.h"

namespace qslary
{
  // Config::ConfigVarMap Config::GetDatas();

  ConfigBase::ptr Config::LookUpBase(const std::string &name)
  {
    qslary::ReadLockGuard lock(GetMutex());
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
  }

  void listAllMember(const std::string &prefix, const YAML::Node &root,
                     std::list<std::pair<std::string, const YAML::Node>> &output)
  {

    if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz_.0123456789") != std::string::npos)
    {
      QSLARY_LOG_ERROR(QSLARY_LOG_ROOT()) << " config invalid name " << prefix << " : " << root;
      return;
    }

    output.push_back(std::make_pair(prefix, root));
    if (root.IsMap())
    {
      for (auto it = root.begin(); it != root.end(); it++)
      {
        listAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
      }
    }
  }

  void Config::loadFromYaml(const YAML::Node &root)
  {

    std::list<std::pair<std::string, const YAML::Node>> all_nodes;

    // 列出所有的YAML::Node 节点
    listAllMember("", root, all_nodes);
    // std::cout<<all_nodes.size()<<std::endl;
    // for(auto i: all_nodes){
    //   std::cout<<i.first<<std::endl;
    // }

    // key是prefix
    for (auto &it : all_nodes)
    {
      std::string key = it.first;
      if (key.empty())
        continue;

      std::transform(key.begin(), key.end(), key.begin(), ::tolower);
      ConfigBase::ptr val = Config::LookUpBase(key);

      if (val)
      {
        if (it.second.IsScalar())
        {
          val->formString(it.second.Scalar());
        }
        else
        {

          std::stringstream ss;

          // it指向一个YAML节点 表示配置文件
          ss << it.second;
          // std::cout << std::endl<<"++++++++++++++++++++++++++++++++++" <<std::endl<<std::endl;
          // std::cout<<"stringstream is "<<std::endl<<std::endl;
          // std::cout<<ss.str()<<std::endl;
          // std::cout<<std::endl<<"++++++++++++++++++++++++++++++++++"<<std::endl<<std::endl;
          val->formString(ss.str());
        }
      }
    }
  }

  void Config::Visit(std::function<void(ConfigBase::ptr)> cb)
  {
    qslary::ReadLockGuard lock(GetMutex());
    ConfigVarMap &m = GetDatas();

    for (auto it = m.begin(); it != m.end(); it++)
    {
      cb(it->second);
    }
  }

}
