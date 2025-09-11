#include <cppon/c++on.h>
#include <iostream>
#include <cassert>

int main() {
    // Create an empty JSON document
    ch5::cppon root{};

    // Build structure by assigning with paths
    std::cout << "Creating a JSON structure...\n";
    root["/parent/child1"] = "value1";
    root["/parent/child2"] = "value2";

    // Check types and values
    std::cout << "Validating types and values...\n";
    assert(std::holds_alternative<ch5::object_t>(root["/parent"]));
    assert(std::holds_alternative<ch5::string_view_t>(root["/parent/child1"]));
    assert(std::get<ch5::string_view_t>(root["/parent/child1"]) == "value1");

    // Create an array via paths
    std::cout << "Creating an array...\n";
    root["/array/0"] = "item1";
    root["/array/1"] = "item2";
    root["/array/2"] = "item3";

    // Validate array
    assert(std::holds_alternative<ch5::array_t>(root["/array"]));

    // Convert to JSON
    std::cout << "\nTo JSON:\n" << ch5::to_string(root) << std::endl;

    // Parse JSON with an isolated context
    std::cout << "\nParsing a JSON string in an isolated context...\n";
    {
        ch5::cppon parsed_doc = ch5::eval(R"({"name":"John","age":30,"skills":["C++","Python"]})");

        // Access parsed values via absolute paths
        auto name = std::get<std::string_view>(parsed_doc["/name"]);
        auto age = ch5::get_cast<int>(parsed_doc["/age"]);
        auto skill = std::get<std::string_view>(parsed_doc["/skills/0"]);

        std::cout << "Name: " << name << ", Age: " << age << ", Skill: " << skill << std::endl;
    }

    // Root is back to the original one
    std::cout << "\nVerifying that root was restored:\n";
    std::cout << "root['/parent/child1']: "
        << std::get<std::string_view>(root["/parent/child1"]) << std::endl;

    return 0;
}