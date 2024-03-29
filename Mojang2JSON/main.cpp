#include <cstdio>

#include <stdexcept>
#include <fstream>

#include "Parsing.h"

/* Writes the parsed mappings to disk in a clean JSON format */
bool writeMappingsAsJSON(Parsing::MappingsMap& mappings)
{
	printf("[+] Writing mappings to \"mappings.json\"\n");

	std::ofstream output_stream("mappings.json", std::ios::trunc | std::ios::binary);

	std::string padding{ "\x20\x20" };
	std::string double_padding{ padding + padding };
	std::string triple_padding{ double_padding + padding };

	output_stream << "{\n";

	for (auto jclass_it{ mappings.begin() }; jclass_it != mappings.end(); jclass_it++)
	{
		auto& jclass{ jclass_it->second };

		output_stream << padding + "\"" + jclass_it->first + "\": {\n";
		output_stream << double_padding + "\"fields\": {\n";

		for (auto jfield_it{ jclass.fields.begin() }; jfield_it != jclass.fields.end(); jfield_it++)
		{
			output_stream << triple_padding + "\"" + jfield_it->getName() + "\": \"U|" + jfield_it->getType() + "|" + jfield_it->getObfuscatedName() + "\"" + (std::next(jfield_it) == jclass.fields.end() ? "\n" : ",\n");
		}

		output_stream << double_padding + "},\n";
		output_stream << double_padding + "\"methods\": {\n";

		for (auto jmethod_it{ jclass.methods.begin() }; jmethod_it != jclass.methods.end(); jmethod_it++)
		{
			output_stream << triple_padding + "\"" + jmethod_it->getName() + "\": \"U|" + "|(";

			for (auto& argument : jmethod_it->getArguments()) output_stream << argument;

			output_stream << ")" + jmethod_it->getType() + "|" + jmethod_it->getObfuscatedName() + "\"" + (std::next(jmethod_it) == jclass.methods.end() ? "\n" : ",\n");
		}

		output_stream << double_padding + "}\n";
		output_stream << padding + "}" + (std::next(jclass_it) == mappings.end() ? "\n" : ",\n");
	}

	output_stream << "}";

	return false;
}

/* Resolves the static indicator for mappings */
bool resolveMappings(Parsing::MappingsMap& mappings)
{
	printf("[+] Resolving mappings' indicator\n");

#ifndef _WIN32
	printf("[!!] Resolving not supported, skipping\n");
#else
	printf("[!!] Resolving not supported, skipping\n");
#endif

	return false;
}

/* Reads and parses mappings from Mojang format */
bool parseMappings(const char* mappings_path, Parsing::MappingsMap& mappings)
{
	printf("[+] Parsing mappings\n");

	std::ifstream input_stream(mappings_path, std::ios::binary);

	printf("[++] Reading mappings\n");

	if (!input_stream.is_open())
	{
		printf("[-] Failed to read \"%s\"\n", mappings_path);
		return true;
	}

	std::string line;
	Parsing::JClass* current_jclass{};
	bool is_parsing_methods{};

	while (std::getline(input_stream, line))
	{
		/* Skip if comment */
		if (line[0] == '#')
			continue;

		/* Is a new class */
		if (line[0] != '\x20')
		{
			Parsing::JClass new_jclass(line);

			mappings.emplace(new_jclass.getName(), new_jclass);
			current_jclass = &mappings[new_jclass.getName()];
			is_parsing_methods = false;
			continue;
		}

		if (current_jclass == nullptr)
		{
			printf("[-] Failed to read mappings, current_class was nullptr\n");
			return true;
		}

		Parsing::cleanLine(line);

		if (is_parsing_methods || line.find('(') != std::string::npos)
		{
			current_jclass->methods.push_back(line);
			is_parsing_methods = true;
		}
		else
			current_jclass->fields.push_back(line);
	}

	input_stream.close();

	printf("[++] Resolving mappings' types\n");

	for (auto jclass_it{ mappings.begin() }; jclass_it != mappings.end(); jclass_it++)
	{
		auto& jclass{ jclass_it->second };

		for (auto jfield_it{ jclass.fields.begin() }; jfield_it != jclass.fields.end(); jfield_it++)
		{
			jfield_it->resolveType(mappings);
		}

		for (auto jmethod_it{ jclass.methods.begin() }; jmethod_it != jclass.methods.end(); jmethod_it++)
		{
			jmethod_it->resolveType(mappings);
			jmethod_it->resolveArgumentsTypes(mappings);
		}
	}

	return false;
}

int main(int argc, char** argv)
{
	try
	{
		if (argc != 2)
			throw std::invalid_argument("Format: <Mojang2JSON> <mappings.txt>");

		Parsing::MappingsMap mappings;

		if (parseMappings(argv[1], mappings))
			throw std::runtime_error("Failed to parse mojang mappings");

		if (resolveMappings(mappings))
			throw std::runtime_error("Failed to resolve mappings");

		if (writeMappingsAsJSON(mappings))
			throw std::runtime_error("Failed to write mappings to JSON to disk");

		printf("[+] Done\n");
	}
	catch (const std::exception& e)
	{
		printf("[-] %s\n", e.what());
		return -1;
	}

	return 0;
}