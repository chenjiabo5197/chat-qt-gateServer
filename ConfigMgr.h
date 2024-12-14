#pragma once

#include "const.h"

struct SectionInfo
{
	SectionInfo() {};
	~SectionInfo() { _section_data.clear(); };

	SectionInfo(const SectionInfo& src)
	{
		_section_data = src._section_data;
	};

	SectionInfo& operator = (const SectionInfo& src)
	{
		// 不允许自己拷贝自己
		if (&src == this)
		{
			return *this;
		}
		this->_section_data = src._section_data;
	};

	std::map<std::string, std::string> _section_data;

	// 重载[]运算符，为了防止没有对应的key报错
	std::string operator[](const std::string& key)
	{
		if (_section_data.find(key) == _section_data.end())
		{
			return "";
		}
		return _section_data[key];
	}
};

class ConfigMgr
{
public:
	~ConfigMgr()
	{
		_config_map.clear();
	};

	// 重载[],也是为了防止在key不存在时出错
	SectionInfo operator[](const std::string& section)
	{
		if (_config_map.find(section) == _config_map.end())
		{
			return SectionInfo();
		}
		return _config_map[section];
	}

	ConfigMgr();

	ConfigMgr(const ConfigMgr& src)
	{
		_config_map = src._config_map;
	};

	ConfigMgr& operator = (const ConfigMgr& src)
	{
		// 不允许自己拷贝自己
		if (&src == this)
		{
			return *this;
		}
		this->_config_map = src._config_map;
	};

private:
	std::map<std::string, SectionInfo> _config_map;
};

