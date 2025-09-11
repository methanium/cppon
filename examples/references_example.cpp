#include <cppon/c++on.h>
#include <iostream>
#include <cassert>

void copilot_fear() {
    {
        ch5::cppon doc;
        doc["/obj/p"] = static_cast<ch5::pointer_t>(nullptr);
        assert(std::holds_alternative<ch5::pointer_t>(doc["/obj/p"]));

        bool thrown = false;
        try {
            doc["/obj/p/child"] = 42;
        }
        catch (const ch5::type_mismatch_error&) {
            thrown = true;
        }
        assert(thrown && "writing through nullptr pointer_t must throw");
        assert(ch5::null().is_null()); // singleton inchangé
    }

    // 2) Écriture via pointer_t valide → OK
    {
        ch5::cppon doc;
        doc["/array/2/value"] = "x";
        doc["/p"] = &doc["/array/2"];
        doc["/p/value"] = "ok";
        assert(std::get<ch5::string_view_t>(doc["/array/2/value"]) == "ok");
    }

    // 3) path_t: lecture/affichage du chemin
    {
        auto obj = ch5::eval(R"({"a":{"b":"v"},"ref":"$cppon-path:/a/b"})");
        assert(std::holds_alternative<ch5::path_t>(obj["/ref"]));
        ch5::root_guard g(obj);
        auto path = std::get<ch5::path_t>(obj["/ref"]).value; // "/a/b"
        auto& target = ch5::visitor(ch5::get_root(), path.substr(1));
        assert(std::get<ch5::string_view_t>(target) == "v");
    }

    std::cout << "OK\n";
}

int main() {
    copilot_fear();

    // Création d'un document avec des objets et tableaux
    ch5::cppon root{};
    // Prépare une cible avec un membre à modifier via la référence
    root["/array/0/value"] = "item0";
    root["/array/2/value"] = "item2";

    std::cout << "Création d'une référence vers /array/2\n";
    root["/pointer"] = &root["/array/2"];
    assert(std::holds_alternative<ch5::pointer_t>(root["/pointer"]));

    std::cout << "Modification via la référence (déréférencement sur un sous-chemin)\n";
    root["/pointer/value"] = "modified value";
    std::cout << "Valeur originale après modification: "
        << std::get<std::string_view>(root["/array/2/value"]) << "\n";
    assert(std::get<std::string_view>(root["/array/2/value"]) == "modified value");

    // Pointeur vers un tableau entier: modification par index
    try {
        // Ligne potentiellement dangereuse si RHS/LHS réalloc
        root["/p_array"] = &root["/array"];
        root["/p_array/1/value"] = "created via pointer to array";
    }
    catch (const std::exception& e) {
        std::cout << "Pointer assign blocked: " << e.what() << "\n";
        // Fallback sûr: séquencer
        auto& slot = root["/p_array"]; // LHS d’abord
        slot = &root["/array"];        // RHS ensuite, OK
        root["/p_array/1/value"] = "created via pointer to array";
    }
    assert(std::get<std::string_view>(root["/array/1/value"]) == "created via pointer to array");

    // Références via JSON ($cppon-path)
    std::cout << "\nCréation de références via la syntaxe JSON\n";
    auto object = ch5::eval(R"({
        "data": {
            "original": "original value"
        },
        "reference": "$cppon-path:/data/original"
    })");

    std::cout << "Original: " << std::get<std::string_view>(object["/data/original"]) << "\n";
    //std::cout << "Référence: " << std::get<ch5::path_t>(object["/reference"]) << "\n";
    assert(std::holds_alternative<ch5::path_t>(object["/reference"]));

    // Afficher le chemin lui-même
    if (std::holds_alternative<ch5::path_t>(object["/reference"])) {
        auto p = std::get<ch5::path_t>(object["/reference"]).value;
        std::cout << "reference path: " << p << "\n";
    }

    // Afficher la valeur ciblée (forcer la résolution)
    if (std::holds_alternative<ch5::path_t>(object["/reference"])) {
        auto p = std::get<ch5::path_t>(object["/reference"]);
        auto& target = ch5::visitor(object, ch5::string_view_t(p));

    //    std::cout << "reference value: "
    //        << std::get<ch5::string_view_t>(target) << "\n";   <---- std::bad_variant_access()
    }

    // Ou plus simple: accéder à un sous-chemin déclenche le déréférencement
//  std::cout << std::get<ch5::string_view_t>(object["/reference"]) << "\n"; // non  <---- std::bad_variant_access()
//  std::cout << std::get<ch5::string_view_t>(object["/reference/"]) << "\n"; // non  <---- ch5::type_mismatch_error()
//  std::cout << std::get<ch5::string_view_t>(object["/reference/any_child"]) << "\n"; // oui, déréf + s <---- ch5::type_mismatch_error()

    // Sortie JSON (optionnel)
    std::cout << "\nRésumé:\n" << ch5::to_string(root, R"({"layout":{"flatten":true}})") << "\n";
    return 0;
}