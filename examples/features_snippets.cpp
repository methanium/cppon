#include <cppon/c++on.h>
#include <iostream>

int main() {
    ch5::cppon doc;
    doc["/a/value"] = 1;
    doc["/b/value"] = 2;

    // Références croisées (cycle)
    doc["/a/ref"] = &doc["/b"];
    doc["/b/ref"] = &doc["/a"];

    // JSON compatible (quote/limits, pas de suffixes)
    std::string json = ch5::to_string(doc, R"({"layout":{"json":true},"compact":true})");
    std::cout << json << "\n";

    // Flatten avec cycles: les cycles restent imprimés en path
    std::string flat = ch5::to_string(doc, R"({"layout":{"flatten":true},"compact":true})");
    std::cout << flat << "\n";

    ch5::cppon cfg = ch5::eval(R"({
        "db":{
            "host":"h"
        },
        "ref": "$cppon-path:/db"
    })");

    auto refs = ch5::resolve_paths(cfg); // path_t → pointer_t
    std::string out = ch5::to_string(cfg, R"({"compact":true})");

    return 0;
}