#include <cppon/c++on.h>
#include <iostream>

using namespace ch5;
using namespace ch5::literals;

int main() {
    // JSON (quick) via UDL
    auto doc = R"({"user":{"name":"Ada","age":37}})"_json;

    // Options (pretty + json-compatible)
    auto opts = R"({"pretty":true,"layout":{"json":true}})"_opts;

    // Path + blobs
    auto p = "/user/name"_path;          // path_t
    auto b64 = "SGVsbG8gd29ybGQh"_b64;     // blob_string_t (Base64)
    auto bin = "\x00\x01""ABC"_blob;         // blob_t (binaire)

    doc["/meta/b64"] = b64;
    doc["/meta/bin"] = bin;

    // Accès typé standard (std::get/std::get_if)
    auto name = std::get<string_view_t>(doc["/user/name"]);
    std::cout << "name=" << name << "\n";

    // Sérialise sous-arbre (compact par défaut), puis pretty/json-compatible
    std::cout << "sub=" << to_string(doc["/user"]) << "\n";
    std::cout << "full=\n" << to_string(doc, opts) << "\n";

    // Réaliser b64 -> blob_t (non-const requis)
    auto& b64v = doc["/meta/b64"];
    auto& binv = doc["/meta/bin"];
    auto& b64dec = get_blob(b64v);  // décode "SGVsbG8..." -> "Hello world!"
    const auto& binref = std::get<blob_t>(binv);

    // Vérifs rapides
    assert((std::string{ reinterpret_cast<const char*>(b64dec.data()), b64dec.size() } == "Hello world!"));
    assert((binref.size() == 5 && binref[0] == 0x00 && binref[1] == 0x01 && binref[2] == 'A' && binref[3] == 'B' && binref[4] == 'C'));
    return 0;
}