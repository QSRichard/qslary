#ifndef __QSLARY_CONFIG_H
#define __QSLARY_CONFIG_H

#include <memory>
#include <string>
#include <sstream>
#include <functional>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <list>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include "logger.h"
#include "util.h"
namespace qslary
{

    class ConfigBase
    {
    public:
        typedef std::shared_ptr<ConfigBase> ptr;
        ConfigBase(const std::string &name, const std::string &description = "") : m_name(name), m_description(description){};
        const std::string &getName() const { return m_name; };
        const std::string getDescription() const { return m_description; };
        virtual ~ConfigBase(){};
        virtual std::string toString() = 0;
        virtual bool formString(const std::string &val) = 0;
        virtual std::string getTypeName() const = 0;

    protected:
        std::string m_name;
        std::string m_description;
    };

    // 从from类型转为to类型
    template <class From, class To>
    class LexicalCast
    {
    public:
        To operator()(const From &val)
        {
            return boost::lexical_cast<To>(val);
        }
    };

    // 从std::string类型转为std::vector<T>类型
    template <class T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        typename std::vector<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            // 注意次数的写法
            typename std::vector<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    // 从std::string类型转为std::list<T>类型
    template <class T>
    class LexicalCast<std::string, std::list<T>>
    {
    public:
        typename std::list<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::list<T> list;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                list.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return list;
        }
    };

    // 从std::string类型转为std::set<T>类型
    template <class T>
    class LexicalCast<std::string, std::set<T>>
    {
    public:
        typename std::set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::set<T> set;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                set.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return set;
        }
    };

    // 从std::string类型转为std::unordered_set<T>类型
    template <class T>
    class LexicalCast<std::string, std::unordered_set<T>>
    {
    public:
        typename std::unordered_set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_set<T> unordered_set;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                unordered_set.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return unordered_set;
        }
    };

    // 从std::string类型转为std::map<std::string,T>类型
    template <class T>
    class LexicalCast<std::string, std::map<std::string, T>>
    {
    public:
        typename std::map<std::string, T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::map<std::string, T> map;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); it++)
            {
                ss.str("");
                ss << it->second;
                map.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }
            return map;
        }
    };

    // 从std::string类型转为std::unordered_map<std::string,T>类型
    template <class T>
    class LexicalCast<std::string, std::unordered_map<std::string, T>>
    {
    public:
        typename std::unordered_map<std::string, T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string, T> unordered_map;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); it++)
            {
                ss.str("");
                ss << it->second;
                unordered_map.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }
            return unordered_map;
        }
    };

    // 从std::vector<T>类型转为std::string类型
    template <class T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T> &v)
        {
            // TAG 是否可以直接输入到std::stringstream中
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 从std::list<T>类型转为std::string类型
    template <class T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::string operator()(const std::list<T> &v)
        {
            // TAG 是否可以直接输入到std::stringstream中
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 从std::set<T>类型转为std::string类型
    template <class T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::string operator()(const std::set<T> &v)
        {
            // TAG 是否可以直接输入到std::stringstream中
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 从std::unordered_set<T>类型转为std::string类型
    template <class T>
    class LexicalCast<std::unordered_set<T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_set<T> &v)
        {
            // TAG 是否可以直接输入到std::stringstream中
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 从std::map<std::string,T>类型转为std::string类型
    template <class T>
    class LexicalCast<std::map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::map<std::string, T> &v)
        {
            YAML::Node node;
            for (auto it = v.begin(); it != v.end(); it++)
            {
                node[it->first] = YAML::Load(LexicalCast<T, std::string>()(it->second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 从std::unordered_map<std::string,T>类型转为std::string类型
    template <class T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_map<std::string, T> &v)
        {
            YAML::Node node;
            for (auto it = v.begin(); it != v.end(); it++)
            {
                node[it->first] = YAML::Load(LexicalCast<T, std::string>()(it->second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // FromStr从std::string类型转为T类型
    // ToStr从T类型转为std::string类型
    template <class T, class FromStr = LexicalCast<std::string, T>, class Tostr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigBase
    {
    public:
        typedef std::shared_ptr<ConfigVar> ptr;
      
        typedef std::function<void(const T &old_value, const T &new_value)> on_change_cb;

        // 注意默认参数只能放在最后一个
        ConfigVar(const std::string &name, const T &default_value, const std::string &description = "") : ConfigBase(name, description), m_val(default_value) {}

        const T getValue() const { return m_val; };

        // TAG 在setValue 函数中调用监听器
        void setValue(const T &v)
        {
            if (v == m_val)
            {
                return;
            }
            for (auto &i : m_cbs)
            {
                i.second(m_val, v);
            }
            m_val = v;
        };

        virtual std::string getTypeName() const override
        {
            return typeid(T).name();
        };

        virtual std::string toString() override
        {
            try
            {
                return Tostr()(m_val);
            }
            catch (std::exception &e)
            {
                QSLARY_LOG_ERROR(QSLARY_LOG_ROOT()) << "Config::toString exception" << e.what() << "convert: " << DEMANGLED_CLASS_NAME(&m_val) << " to std::string ";
            }
            return "";
        };

        virtual bool formString(const std::string &val) override
        {
            try
            {
                setValue(FromStr()(val));
                return true;
            }
            catch (std::exception &e)
            {
                QSLARY_LOG_ERROR(QSLARY_LOG_ROOT()) << "Config::fromString exception " << e.what() << " convert string to " << DEMANGLED_CLASS_NAME(&m_val);
            }
            return false;
        }

        void addListener(uint64_t key, on_change_cb cb)
        {
            m_cbs[key] = cb;
        }

        void delListener(uint64_t key)
        {
            m_cbs.erase(key);
        }

        void clearLister()
        {
            m_cbs.clear();
        }

        on_change_cb getListener(uint64_t key)
        {
            auto it = m_cbs.find(key);
            if (it != m_cbs.end())
            {
                return it->second;
            }
            return nullptr;
        }

    private:
        T m_val;
        // 回调函数数组
        std::map<uint64_t, on_change_cb> m_cbs;
    };

    class Config
    {
    public:
        typedef std::map<std::string, ConfigBase::ptr> ConfigVarMap;

        template <class T>
        static typename ConfigVar<T>::ptr lookup(const std::string &name, const T &default_value, const std::string &description = "")
        {

            auto it = GetDatas().find(name);
            if (it != GetDatas().end())
            {

                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                if (tmp)
                {
                    QSLARY_LOG_INFO(QSLARY_LOG_ROOT()) << "Lookup name = " << name << " is exists";
                    return tmp;
                }
                else
                {
                    QSLARY_LOG_ERROR(QSLARY_LOG_ROOT()) << "Lookup name = " << name << " is exists but type is not " << typeid(T).name() << " real type is " << it->second->getTypeName() << " value is " << it->second->toString();
                    return nullptr;
                }
            }
            // 应使用上面的代码来判断是否已经存在name相同的配置，否则会出现同名但是类型不同的配置项同时存在
            // auto it=lookup<T>(name);
            // if(it){
            //     QSLARY_LOG_INFO(QSLARY_LOG_ROOT())<<"Lookup name = "<<name<<" is exists";
            //     return it;
            // }
            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.0123456789") != std::string::npos)
            {
                QSLARY_LOG_ERROR(QSLARY_LOG_ROOT()) << "Lookup name is invalid" << name;
                throw std::invalid_argument(name);
            }
            // std::cout<<"lookup function will add "<<std::endl;
            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            GetDatas()[name] = v;
            return v;
        }

        template <class T>
        static typename ConfigVar<T>::ptr lookup(const std::string &name)
        {
            auto it = GetDatas().find(name);
            if (it == GetDatas().end())
            {
                return nullptr;
            }
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }
        static ConfigBase::ptr LookUpBase(const std::string &name);
        static void loadFromYaml(const YAML::Node &root);

    private:
        static ConfigVarMap &GetDatas()
        {
            static ConfigVarMap s_datas;
            return s_datas;
        };
    };
}

#endif