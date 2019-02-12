#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <cassert>

#ifndef _UTIL_H
#define _UTIL_H

// Used in matrix manipulations 
typedef unsigned char BYTE_T;
typedef std::vector<BYTE_T> BYTEVEC_T;
typedef std::pair<unsigned int, unsigned int> COORD_T;

// Used in entropy encoding and compression manipulations
typedef std::vector<int> INT_VEC_T;

typedef std::map<std::string, std::string> CONFIG_MAP_T;

class CFG
{
private:
	CONFIG_MAP_T m_config_map;
	CFG() {};
	
public:
	static CFG& inst() {
		static CFG instance;
		return instance;
	}
	
	void init(const char* filename);
	bool load_opt(const std::string& opt, std::string& str_val);
	bool load_opt(const std::string& opt, int& int_val);
	bool load_opt(const std::string& opt, unsigned int& uint_val);
	bool load_opt(const std::string& opt, bool& bool_val);
};

//Helpful macros for loading in mandatory settings, tracking their success, and printing an error when they're missing
#define CFG_LOAD_OPT_MANDATORY(name, var, ret)	\
if (!CFG::inst().load_opt(name, var)) { ret = false; std::cout << "ERROR: Missing cfg opt \"" << name << "\"!" << std::endl; }

#define CFG_LOAD_OPT_DEFAULT(name, var, def)	\
if (!CFG::inst().load_opt(name, var)) { var = def; }

class DEBUG_CSV
{
private: 
	DEBUG_CSV(const char* fname) : m_ofstream(fname) { assert(m_ofstream.is_open()); }
	~DEBUG_CSV() { m_ofstream.close(); }
	
	template <typename T>
	void push_row(const std::vector<T>& row)
	{
		unsigned int i;
		for(i=0; i < row.size(); ++i)
		{
			m_ofstream << row[i];
			if(i < row.size() -1)
				m_ofstream << ',';
		}
		m_ofstream << std::endl;
	}
		
	static DEBUG_CSV* get_inst()
	{
		static DEBUG_CSV csv("debug.csv");
		return &csv;
	}
	
	std::ofstream m_ofstream;
	
public:
	template <typename T>
	static void push(const std::vector<T>& row)
	{
		get_inst()->push_row<T>(row);
	}

};

#endif //_UTIL_H