#include <cppon/c++on.h>
#include <iostream>
#include <cassert>

int main() {
    // Cr�ation d'un document avec une structure complexe
    ch5::cppon doc{};

    std::cout << "=== Cr�ation d'�l�ments via des chemins ===\n";

    // Cr�ation de structure imbriqu�e via des chemins absolus
    doc["/users/admin/name"] = "Administrator";
    doc["/users/admin/permissions/0"] = "read";
    doc["/users/admin/permissions/1"] = "write";
    doc["/users/admin/permissions/2"] = "execute";

    doc["/users/guest/name"] = "Guest";
    doc["/users/guest/permissions/0"] = "read";

    doc["/settings/theme"] = "dark";
    doc["/settings/language"] = "fr";

    // Cr�ation de tableau multi-dimensionnel via des chemins
    doc["/matrix/0/0"] = 1;
    doc["/matrix/0/1"] = 2;
    doc["/matrix/1/0"] = 3;
    doc["/matrix/1/1"] = 4;

    std::cout << "Structure cr��e!\n\n";

    // Afficher la structure compl�te
    std::cout << "Document JSON complet:\n" << ch5::to_string(doc, "{\"compact\":false}") << "\n\n";

    std::cout << "=== Acc�s via chemins absolus ===\n";

    // Acc�s direct via chemins absolus
    auto admin_name = std::get<std::string_view>(doc["/users/admin/name"]);
    auto permission = std::get<std::string_view>(doc["/users/admin/permissions/0"]);
    auto theme = std::get<std::string_view>(doc["/settings/theme"]);

    std::cout << "Nom admin: " << admin_name << "\n";
    std::cout << "Permission: " << permission << "\n";
    std::cout << "Th�me: " << theme << "\n\n";

    std::cout << "=== Acc�s via objets interm�diaires et chemins relatifs ===\n";

    // Navigation par �tapes avec des objets interm�diaires
    auto users = doc["/users"];
    auto guest = users["guest"];           // Chemin relatif sans /
    auto guest_name = guest["name"];       // Chemin relatif sans /

    std::cout << "Nom invit� (acc�s progressif): "
        << std::get<std::string_view>(guest_name) << "\n";

    // M�lange de chemins absolus et relatifs
    auto settings = doc["/settings"];
    auto language = settings["language"];  // Chemin relatif

    std::cout << "Langue (chemin relatif): "
        << std::get<std::string_view>(language) << "\n";

    // Acc�s aux �l�ments d'un tableau
    auto matrix = doc["/matrix"];
    auto cell = matrix["1/0"];  // Chemin relatif pour [1][0]

    std::cout << "Cellule matrix[1][0]: "
        << ch5::get_cast<int>(cell) << "\n\n";

    std::cout << "=== Acc�s absolu depuis n'importe quel niveau ===\n";

    // Acc�s absolu depuis un objet interm�diaire
    auto absolute_from_nested = guest["/settings/theme"];

    std::cout << "Th�me (acc�s absolu depuis guest): "
        << std::get<std::string_view>(absolute_from_nested) << "\n";

    std::cout << "\n=== Modification via chemins ===\n";

    // Modification via chemins
    doc["/settings/theme"] = "light";

    std::cout << "Nouveau th�me: "
        << std::get<std::string_view>(doc["/settings/theme"]) << "\n";

    // Cr�ation d'un nouveau chemin
    doc["/settings/notifications/email"] = true;

    std::cout << "Nouvelle option cr��e: "
        << (std::get<bool>(doc["/settings/notifications/email"]) ? "activ�e" : "d�sactiv�e") << "\n";

    return 0;
}