#include "Parsing.h"

using namespace Parsing;

//
// Utilities
//

void Parsing::cleanLine(std::string& line)
{
	if (size_t line_it; (line_it = line.find_last_of(':')) != std::string::npos)
		line = line.substr(line_it + 1);
	else
		line = line.substr(4);
}

//
// Class JBase
//

void JBase::resolveType(MappingsMap& mappings)
{
	size_t type_it{ type.find('!') };

	/* If no indicator, no need for resolving */
	if (type_it == std::string::npos)
		return;

	type_it++;
	std::string object_name{ type.substr(type_it, type.size() - type_it - 1) };

	if (mappings.find(object_name) == mappings.end())
		return;

	type = "L" + mappings[object_name].getObfuscatedName() + ";";
}

std::string JBase::resolveObjectType(std::string type)
{
	std::string start{ type.substr(0, type.find_first_of('.')) };

	/* If object type is filtered for */
	if (std::find(object_types.begin(), object_types.end(), start) != std::end(object_types))
	{
		std::replace(type.begin(), type.end(), '.', '/');
		return "L" + type + ";";
	}

	// Return it with ! so that it can be resolved after parsing all classes
	return "L!" + type.substr(type.find_last_of(".") + 1) + ";";
}

std::string JBase::resolveLineType(std::string line)
{
	std::string type{ line.substr(0, line.find_first_of('\x20')) };
	bool is_array{ type.find('[') != std::string::npos };

	if (is_array)
		type = type.substr(0, type.size() - 2);

	auto jni_type{ jni_types.find(type) };

	/* If type is not JNI, it's an object */
	if (jni_type == jni_types.end())
		type = (is_array ? "[" : "") + resolveObjectType(type);
	else
		type = (is_array ? "[" : "") + jni_type->second;

	return type;
}

std::string JBase::resolveLineObfuscatedName(std::string line)
{
	return line.substr(line.find_last_of('\x20') + 1);
}

std::string JBase::getName()
{
	return name;
}

std::string JBase::getType()
{
	return type;
}

std::string JBase::getObfuscatedName()
{
	return obfuscated_name;
}

//
// Class JField
//

JField::JField(const std::string& line)
{
	type = resolveLineType(line);

	/* Read name */
	{
		size_t start{ line.find('\x20') + 1 };
		name = line.substr(start, line.find('\x20', start + 1) - start);
	}

	obfuscated_name = resolveLineObfuscatedName(line);
}

//
// Class JMethod
//

JMethod::JMethod(const std::string& line)
{
	type = resolveLineType(line);

	/* Read name & arguments */
	{
		size_t start{ line.find('\x20') + 1 };
		size_t end{ line.find('(', start + 1) };
		name = line.substr(start, end - start);

		std::string arguments_line{ line.substr(end, line.find('\x20', end) - end) };
		arguments_line = arguments_line.substr(1, arguments_line.size() - 2);
		std::stringstream arguments_stream{ arguments_line };
		std::string argument;

		while (std::getline(arguments_stream, argument, ',')) arguments.push_back(resolveLineType(argument));
	}

	obfuscated_name = resolveLineObfuscatedName(line);
}

void JMethod::resolveArgumentsTypes(MappingsMap& mappings)
{
	for (auto& argument : arguments)
	{
		size_t argument_it{ argument.find('!') };

		/* If no indicator, no need for resolving */
		if (argument_it == std::string::npos)
			return;

		argument_it++;
		std::string object_name{ argument.substr(argument_it, argument.size() - argument_it - 1) };

		if (mappings.find(object_name) == mappings.end())
			return;

		argument = "L" + mappings[object_name].getObfuscatedName() + ";";
	}
}

std::vector<std::string>& JMethod::getArguments()
{
	return arguments;
}

//
// Class JClass
//

JClass::JClass(const std::string& line)
{
	size_t line_it{ line.find_first_of('\x20') }; // Get end of non-obfuscated name
	name = line.substr(0, line_it);
	resolveName(name);

	line_it = line.find_last_of('\x20') + 1; // Get start of obfuscated name
	obfuscated_name = line.substr(line_it, line.size() - line_it - 1);
}

void JClass::resolveName(std::string& name)
{
	size_t name_it{ name.find_last_of('.') + 1 };
	name = name.substr(name_it);
}

std::string JClass::getName()
{
	return name;
}

std::string JClass::getObfuscatedName()
{
	return obfuscated_name;
}
