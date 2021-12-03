#include "../qslary/config.h"
#include "../qslary/logger.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <vector>

// // int Config
// qslary::ConfigVar<int>::ptr g_int_value_config=
// qslary::Config::lookup("system.port",(int)8080,"system.port");

// qslary::ConfigVar<double>::ptr g_if_value_config =
// qslary::Config::lookup("system.port", (double)9908, "system.port");

// // float Config
// qslary::ConfigVar<float>::ptr g_float_value_config=
// qslary::Config::lookup("system.value",(float)2.999,"system.value");

// // vector<int> Config
// qslary::ConfigVar<std::vector<int>>::ptr g_vec_value_config=
// qslary::Config::lookup("system.vec",std::vector<int>(3,4),"system.vec");

// // list<int> Config
// qslary::ConfigVar<std::list<int>>::ptr g_list_int_value_config=
// qslary::Config::lookup("system.list_int",std::list<int>{1,2},"system.list_int");

// // set<int> Config
// qslary::ConfigVar<std::set<int>>::ptr g_set_int_value_config =
// qslary::Config::lookup("system.set_int", std::set<int>{1, 2}, "system.set_int");

// // unordered_set<int> Config
// qslary::ConfigVar<std::unordered_set<int>>::ptr g_unordered_set_int_value_config =
// qslary::Config::lookup("system.unordered_set_int", std::unordered_set<int>{9,8,4,9}, "system.unordered_set_int");

// // map<int> Config
// qslary::ConfigVar<std::map<std::string,int>>::ptr g_map_int_value_config =
// qslary::Config::lookup("system.map_int", std::map<std::string,int>{{"qs",23}}, "system.map_int");

// // unordered_map<int> Config
// qslary::ConfigVar<std::unordered_map<std::string, int>>::ptr g_unordered_map_int_value_config =
// qslary::Config::lookup("system.unordered_map_int", std::unordered_map<std::string, int>{{"qs", 23}}, "system.unordered_map_int");

// void test_config(){
//     QSLARY_LOG_INFO(QSLARY_LOG_ROOT())<<"before int: "<<g_int_value_config->getValue();
//     QSLARY_LOG_INFO(QSLARY_LOG_ROOT())<<"before float: "<<g_float_value_config->toString();

/*     #define XX(g_val,name,prefix) \
//     {\
//             auto& val=g_val->getValue(); \
//             for(auto& i:val){ \
//                 QSLARY_LOG_INFO(QSLARY_LOG_ROOT())<<#prefix " " <<#name<<": "<<i; \
//         }\
//     }

//     #define XX_A(g_val,name,prefix) \
//     {\
//         auto& val=g_val->getValue(); \
//         for(auto& i:val){ \
//             QSLARY_LOG_INFO(QSLARY_LOG_ROOT())<<#prefix " "<<#name ": "<<"{ "<<i.first <<" "<<i.second<<"}"; \
//         } \
//         QSLARY_LOG_INFO(QSLARY_LOG_ROOT())<<#prefix " "#name" toString(): "<<g_val->toString();\
        }*/

//     XX(g_vec_value_config,vec_int,before);
//     XX(g_list_int_value_config,list_int,before);
//     XX(g_set_int_value_config,set_int,before);
//     XX(g_unordered_set_int_value_config, unordered_set_int, before);
//     XX_A(g_map_int_value_config,map_int,before);
//     XX_A(g_unordered_map_int_value_config,unordered_map,before);

//     YAML::Node root = YAML::LoadFile("/home/liushui/workspace/qslary/bin/config/log.yml");
//     qslary::Config::loadFromYaml(root);

//     QSLARY_LOG_INFO(QSLARY_LOG_ROOT())<<"after int: "<<g_int_value_config->getValue();
//     QSLARY_LOG_INFO(QSLARY_LOG_ROOT())<<"after float: "<<g_float_value_config->toString();

//     XX(g_vec_value_config, vec_int, after);
//     XX(g_list_int_value_config, list_int, after);
//     XX(g_set_int_value_config, set_in, after);
//     XX(g_unordered_set_int_value_config, unordered_set_int, after);
//     XX_A(g_map_int_value_config, map_int, after);
//     XX_A(g_unordered_map_int_value_config, unordered_map, after);
// }

void test_log()
{

  static qslary::Logger::ptr system_log = QSLARY_LOG_NAME("system");
  QSLARY_LOG_INFO(system_log) << "hello system" << std::endl
                              << std::endl;

  std::cout << "LoggerMgr::getInstance()..." << std::endl
            << std::endl;
  ;
  std::cout << qslary::LoggerMgr::getInstance()->toYamlString() << std::endl
            << std::endl;

  std::cout << "LoadFile..." << std::endl
            << std::endl;
  YAML::Node root = YAML::LoadFile("/home/liushui/workspace/qslary/bin/config/log.yml");

  std::cout << "loadFromYaml()..." << std::endl
            << std::endl;
  qslary::Config::loadFromYaml(root);

  std::cout << "again call LoggerMgr::getInstance()->toYamlString()... 反序列化" << std::endl
            << std::endl;
  std::cout << "==========================" << std::endl;
  std::cout << qslary::LoggerMgr::getInstance()->toYamlString() << std::endl
            << std::endl;
  std::cout << "==========================" << std::endl;

  std::cout << root << std::endl
            << std::endl;
  QSLARY_LOG_INFO(system_log) << "hello system" << std::endl
                              << std::endl;
  system_log->setFormatter("%d - %m%n");
  QSLARY_LOG_INFO(system_log) << "hello system" << std::endl
                              << std::endl;
}

int main()
{

  // QSLARY_LOG_INFO(QSLARY_LOG_ROOT())<<g_int_value_config->getValue();
  // QSLARY_LOG_INFO(QSLARY_LOG_ROOT()) << g_int_value_config->toString();
  // test_yaml();

  // test_config();

  test_log();
  return 0;
}

// void print_yaml(const YAML::Node &root, int level)
// {
//     if (root.IsNull())
//     {
//         QSLARY_LOG_INFO(QSLARY_LOG_ROOT()) << std::string(4 * level, ' ') << "Null - " << root.Type() << " - " << level;
//     }
//     else if (root.IsScalar())
//     {
//         QSLARY_LOG_INFO(QSLARY_LOG_ROOT()) << std::string(4 * level, ' ') << root.Scalar() << " - " << root.Type() << " - " << level;
//     }
//     else if (root.IsMap())
//     {
//         for (auto it = root.begin(); it != root.end(); it++)
//         {
//             QSLARY_LOG_INFO(QSLARY_LOG_ROOT()) << std::string(4 * level, ' ') << it->first << " - " << it->second.Type() << " - " << level;
//             print_yaml(it->second, level + 1);
//         }
//     }
//     else if (root.IsSequence())
//     {
//         for (size_t i = 0; i < root.size(); i++)
//         {
//             QSLARY_LOG_INFO(QSLARY_LOG_ROOT()) << std::string(4 * level, ' ') << i << " - " << root[i].Type() << " - " << level;
//             print_yaml(root[i], level + 1);
//         }
//     }
// }

// void test_yaml()
// {
//     YAML::Node node = YAML::LoadFile("/home/liushui/workspace/qslary/bin/config/log.yml");
//     print_yaml(node, 0);
//     // QSLARY_LOG_INFO(QSLARY_LOG_ROOT())<<node;
// }