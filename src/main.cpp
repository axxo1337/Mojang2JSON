#include <stdexcept>
#include <map>
#include <string>
#include <tuple>
#include <fstream>
#include <algorithm>
#include <set>

#include <cstdio>
#include <cstdlib>
#include <cstring>

std::map<std::string, std::string> classes;

const std::map<std::string, std::string> types = {
	{"boolean", "Z"},
	{"byte", "B"},
	{"char", "C"},
	{"short", "S"},
	{"int", "I"},
	{"long", "J"},
	{"float", "F"},
	{"double", "D"},
	{"void", "V"},
};

/* Removes garbage mojang stuff */
void cleanMapping(std::string &mapping)
{
	size_t it_begin{mapping.find_last_of(':')};

	if (it_begin != std::string::npos)
	{
		it_begin += 1;
		mapping = mapping.substr(it_begin, mapping.size() - it_begin);
	}
	else
		mapping = mapping.substr(4, mapping.size() - 4);
}

struct JField
{
	std::string name;
	std::string jni_name;
	std::string type;

	JField(std::string &mapping)
	{
		cleanMapping(mapping);

		// TODO MAKE TYPE PARSER
		size_t it_end{mapping.find_first_of('\x20')};
		type = mapping.substr(0, it_end);

		size_t it_begin{it_end + 1};
		it_end = mapping.find_first_of('\x20', it_begin);
		name = mapping.substr(it_begin, it_end - it_begin);

		it_begin = mapping.find_first_of('>', it_end) + 2;
		jni_name = mapping.substr(it_begin, mapping.size() - (it_begin));
	}

	static void parseType(std::string &type)
	{
		size_t begin{type.find_first_of('[')};
		bool is_array{begin != std::string::npos};

		if (is_array)
			type = type.substr(0, begin);

		/* Check if not found */
		if (types.find(type) == types.end())
		{
			size_t begin{type.find_last_of('.') + 1};
			std::string class_name{type.substr(begin, type.size() - begin)};

			/* Check if class was parsed */
			if (classes.find(class_name) != classes.end())
				type = "L" + classes.at(class_name) + ";";
			else
			{
				std::replace(type.begin(), type.end(), '.', '/');
				type = "L" + type + ";";
			}

			return;
		}

		if (is_array)
			type = "[" + types.at(type);
		else
			type = types.at(type);
	}
};

struct JMethod
{
	std::string name;
	std::string jni_name;
	std::string signature;

	JMethod(std::string &mapping)
	{
		cleanMapping(mapping);

		// TODO MAKE TYPE PARSER
		size_t it_end{mapping.find_first_of('\x20')};
		signature = mapping.substr(0, it_end);

		size_t it_begin{it_end + 1};
		it_end = mapping.find_first_of('(', it_begin);
		name = mapping.substr(it_begin, it_end - it_begin);

		it_begin = it_end;
		it_end = mapping.find_first_of(')', it_begin) + 1;
		signature = mapping.substr(it_begin, it_end - it_begin) + signature;

		it_begin = mapping.find_first_of('>', it_end) + 2;
		jni_name = mapping.substr(it_begin, mapping.size() - (it_begin));
	}

	static void parseSignature(std::string &signature)
	{
		size_t end{signature.find_first_of(')')};
		std::string arguments{signature.substr(1, end - 1)};

		/* Check if there are any args */
		if (arguments.size() == 0)
			arguments = "()";
		else
		{
			std::string final_args = "(";

			size_t begin{};
			size_t end{arguments.find_first_of(',')};

			do
			{
				/* Single argument */
				if (end == std::string::npos)
				{
					JField::parseType(arguments);
					break;
				}

				std::string arg{arguments.substr(begin, end - begin)};
				JField::parseType(arg);

				final_args += arg;

				begin = end + 1;
				end = arguments.find_first_of(',', begin);

				/* Last argument */
				if (end == std::string::npos)
				{
					arg = arguments.substr(begin, arguments.size() - begin);
					JField::parseType(arg);
					final_args += arg;
					break;
				}
			} while (end != std::string::npos);

			arguments = final_args + ")";
		}

		end += 1;
		std::string return_type{signature.substr(end, signature.size() - end)};
		JField::parseType(return_type);

		signature = arguments + return_type;
	}
};

struct JClass
{
	std::string name;
	std::string jni_name;

	std::vector<JField> fields;
	std::vector<JMethod> methods;
	bool imperative;

	JClass(std::string &mapping)
	{
		size_t it_begin{mapping.find_last_of('.') + 1};
		size_t it_end{mapping.find_first_of('\x20', it_begin)};

		name = mapping.substr(it_begin, it_end - it_begin);
		std::replace(name.begin(), name.end(), '.', '/');

		it_begin = mapping.find_first_of('>', it_end) + 2;
		jni_name = mapping.substr(it_begin, mapping.size() - (it_begin + 1));

		classes.insert({name, jni_name});
	}
};

/* The option for whitelist should be `--whitelist`, but does not matter */
struct ArgumentsInfo final
{
	char *whitelist_path{}; // if nullptr, does not use mappings
	char *mappings_path{};

	ArgumentsInfo(int argc, char **argv)
	{
		if (argc == 2)
			mappings_path = argv[1];
		else
		{
			whitelist_path = argv[2];
			mappings_path = argv[3];
		}
	}
};

using MappingsList = std::vector<JClass>;

bool convertMappings(MappingsList &mappings, std::string &result)
{
	printf("[+] Converting mappings\n");

	for (auto &jclass : mappings)
	{
		result += jclass.name + "|" + jclass.jni_name + "\n";

		for (auto &jfield : jclass.fields)
			result += "-" + jfield.name + "|" + jfield.jni_name + "|" + jfield.type + "|U" + "\n";

		for (auto &jmethod : jclass.methods)
			result += "--" + jmethod.name + "|" + jmethod.jni_name + "|" + jmethod.signature + "|U" + "\n";
	}

	return false;
}

/* Use whitelist to filter mappings */
bool filterMappings(MappingsList &mappings, const char *whitelist_path)
{
	printf("[+] Filtering mappings\n");

	std::ifstream whitelist_stream(whitelist_path, std::ios::binary);

	if (!whitelist_stream.is_open())
	{
		printf("[-] Failed to open whitelist file\n");
		return true;
	}

	std::set<std::string> whitelist;
	std::string line;

	while (getline(whitelist_stream, line))
	{
		if (whitelist.find(line) != whitelist.end())
		{
			printf("[!] Duplicate item (%s) in whitelist file\n", line.c_str()); // format string vulnerability lol
			continue;
		}

		whitelist.insert(line);
	}

	whitelist_stream.close();

	std::vector<JClass> mappings_copy = mappings;
	mappings.clear();

	for (auto &jclass : mappings_copy)
	{
		if (whitelist.find(jclass.name) != whitelist.end() || whitelist.find(jclass.jni_name) != whitelist.end())
			mappings.push_back(jclass);
	}

	return false;
}

/* Parses the mappings */
bool parseMappings(ArgumentsInfo &args, MappingsList &mappings)
{
	printf("[+] Parsing mappings\n");

	std::ifstream mappings_stream(args.mappings_path, std::ios::binary);

	if (!mappings_stream.is_open())
	{
		printf("[-] Failed to read mappings file\n");
		return true;
	}

	std::string line;
	JClass *curr_class{nullptr};
	bool target_methods{false};

	/* Parse mappings */
	while (getline(mappings_stream, line))
	{
		/* Check if class, field or method */
		if (line[0] != '\x20')
		{
			mappings.emplace_back(JClass(line));
			curr_class = &mappings.back();
			target_methods = false;
		}
		else if (!target_methods && line.find_first_of('(') == std::string::npos)
			curr_class->fields.emplace_back(JField(line));
		else
		{
			curr_class->methods.emplace_back(JMethod(line));
			target_methods = true; // optimizes iteration as mappings always have methods at bottom of class, so until new class, still methods
		}
	}

	mappings_stream.close();

	/* Parse the types */
	for (auto &jclass : mappings)
	{
		for (auto &jfield : jclass.fields)
			jfield.parseType(jfield.type);

		for (auto &jmethod : jclass.methods)
			jmethod.parseSignature(jmethod.signature);
	}

	if (args.whitelist_path)
	{
		if (filterMappings(mappings, args.whitelist_path))
			printf("[!] Failed to filter mappings, bypassing\n");
	}

	std::string converted_mappings;

	if (convertMappings(mappings, converted_mappings))
	{
		printf("[-] Failed to convert mappings");
		return true;
	}

	// TEMP
	std::ofstream output_stream("result_mappings.txt", std::ios::binary | std::ios::trunc);
	output_stream.write(converted_mappings.c_str(), converted_mappings.size());
	output_stream.close();

#ifdef _WIN32
	// TODO: ADD MAPPINGS RESOLVER
#else
	printf("[!] Non-Windows system, will not resolve mappings and will not generate SDK\n");
	return false;
#endif

	return false;
}

int main(int argc, char **argv)
{
	try
	{
		if (argc < 2)
			throw std::invalid_argument("Format: <Mojang2SDK> <option> <inputs>");

		ArgumentsInfo args(argc, argv);

		MappingsList mappings;

		if (parseMappings(args, mappings))
			throw std::runtime_error("Failed to parse mappings");

		printf("[+] Done\n");
	}
	catch (const std::exception &e)
	{
		printf("[-] %s\n", e.what());
		return -1;
	}

	return 0;
}
