#ifndef PARSING_H_
#define PARSING_H_

#include <map>
#include <unordered_map>
#include <vector>
#include <array>
#include <algorithm>
#include <sstream>

namespace Parsing
{
	class JClass;
	using MappingsMap = std::map<std::string, JClass>;

	static const std::unordered_map<std::string, std::string> jni_types = {
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

	static const std::array<std::string, 4> object_types{ "java", "org", "it", "com" };

	void cleanLine(std::string& line);

	class JBase
	{
	public:
		void resolveType(MappingsMap& mappings);

		static std::string resolveObjectType(std::string type);
		static std::string resolveLineType(std::string line);
		static std::string resolveLineObfuscatedName(std::string line);

		std::string getName();
		std::string getType();
		std::string getObfuscatedName();

	protected:
		std::string name;
		std::string type;
		std::string obfuscated_name;
	};

	class JField final : public JBase
	{
	public:
		JField(const std::string& line);
	};

	class JMethod final : public JBase
	{
	public:
		JMethod(const std::string& line);

		void resolveArgumentsTypes(MappingsMap& mappings);

		std::vector<std::string>& getArguments();

	private:
		std::vector<std::string> arguments;
	};

	class JClass
	{
	public:
		JClass() {}
		JClass(const std::string& line);

		static void resolveName(std::string& name);

		std::string getName();
		std::string getObfuscatedName();

		std::vector<JField> fields;
		std::vector<JMethod> methods;

	private:
		std::string name;
		std::string obfuscated_name;
	};
}


#endif