#include <cppon/c++on.h>
#include <iostream>

int main() {
    // Cr�ation d'un document JSON avec plusieurs niveaux
    ch5::cppon doc{};
    doc["/user/name"] = "John Smith";
    doc["/user/email"] = "john@example.com";
    doc["/user/address/street"] = "123 Main St";
    doc["/user/address/city"] = "Anytown";
    doc["/user/address/zip"] = "12345";
    doc["/user/skills/0"] = "C++";
    doc["/user/skills/1"] = "JSON";
    doc["/user/skills/2"] = "SIMD";

    // R�f�rences crois�es
    doc["/departments/engineering/lead"] = &doc["/user"];           // R�f�rence directe
    doc["/projects/0/contributor"] = &doc["/user"];                 // R�f�rence r�p�t�e
    doc["/skills_directory/programming"] = &doc["/user/skills"];    // R�f�rence � un tableau
    doc["/references/user_path"] = "$cppon-path:/user";             // R�f�rence par chemin

    // Options de formatage: Compact
    std::cout << "Format compact:\n";
    std::string compact = ch5::to_string(doc, "{\"compact\":true}");
    std::cout << compact << "\n\n";

    // Options de formatage: Pretty (non-compact)
    std::cout << "Format pretty (non-compact):\n";
    std::string pretty = ch5::to_string(doc, "{\"compact\":false}");
    std::cout << pretty << "\n\n";

    // Options de formatage: Structure aplatie
    std::cout << "Format aplati:\n";
    std::string flat = ch5::to_string(doc, "{\"layout\":{\"flatten\":true}}");
    std::cout << flat << "\n\n";

    // Options combin�es
    std::cout << "Format combin� (aplati + non-compact):\n";
    std::string combined = ch5::to_string(doc, "{\"layout\":{\"flatten\":true,\"compact\":false}}");
    std::cout << combined << "\n\n";

    // Pr�paration des options via eval
    std::cout << "Utilisation d'options pr�par�es:\n";
    auto options = ch5::eval("{\"compact\":true,\"buffer\":\"reserve\"}");
    std::string optimized = ch5::to_string(doc, options);
    std::cout << optimized << "\n";

    return 0;
}