#pragma once
#include <string>
#include <vector>


// scrapped for now
// maybe later with better ideas for implementation

class VertexShaderBuilder {
private:
	enum class Types {

	};
	struct Component {
		Types type;
		std::string name;
	};
	std::string source;
	std::string version;
	std::vector<Component> uniforms;
	std::vector<Component> outputs;
	std::string main_func;


	void parse(std::string& src) {
	}
public:
	VertexShaderBuilder(std::string source) {
		parse(source);
	}
};