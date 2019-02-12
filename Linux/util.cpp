#include "util.h"
#include <fstream>
#include <iostream>
#include <algorithm>

void CFG::init(const char* filename)
{
	std::string line;
	std::ifstream conf_file(filename);
	if(conf_file.is_open())
	{
		while(getline(conf_file, line))
		{
			//Skip comment lines
			if(line[0] == '#')
				continue;
			
			std::size_t sep = line.find_first_of("=");
			
			//Skip lines without any assignments
			if(sep == std::string::npos)
				continue;
			
			std::string opt = line.substr(0, sep);
			std::string val = line.substr(sep+1);
			m_config_map[opt] = val;
		}
	}
	else
	{
		std::cout << "ERROR: No configuration file found!" << std::endl;
	}
}

bool CFG::load_opt(const std::string& opt, std::string& str_opt)
{
	auto it = m_config_map.find(opt);
	bool found = it != m_config_map.end();
	if(found)
	{
		str_opt = it->second;
	}
	return found;
}

bool CFG::load_opt(const std::string& opt, int& int_opt)
{
	auto it = m_config_map.find(opt);
	bool found = it != m_config_map.end();
	if(found)
	{
		int_opt = std::stoi(it->second);
	}
	return found;
}

bool CFG::load_opt(const std::string& opt, unsigned int& uint_opt)
{
	auto it = m_config_map.find(opt);
	bool found = it != m_config_map.end();
	if(found)
	{
		int val = std::stoi(it->second);
		if(val < 0)
		{
			found = false;
		}
		else
		{
			uint_opt = val;
		}
	}
	return found;
}

bool CFG::load_opt(const std::string& opt, bool& bool_val)
{
	auto it = m_config_map.find(opt);
	bool found = it != m_config_map.end();
	if(found)
	{
		std::string val = it->second;
		std::transform(val.begin(), val.end(), val.begin(), toupper);
		if(val == "ON" || val == "1" || val == "TRUE")
		{
			bool_val = true;
		}
		else if (val == "OFF" || val == "0" || val == "FALSE")
		{
			bool_val = false;
		}
		else
		{
			found = false;
		}
	}
	return found;
}
