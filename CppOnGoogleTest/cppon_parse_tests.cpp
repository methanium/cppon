#include "pch.h"
#include <cppon/c++on.h>
#include <cppon/c++on-document.h>
#include <cstdio>
#include <fstream>

using namespace ch5;

TEST(CPPON_Parse, ParsesSimpleInt) {
    auto v = ch5::eval("{\"a\":1}");
    EXPECT_EQ(ch5::get_strict<int64_t>(v["a"]), 1);
}

TEST(CPPON_Parse, ParsesString) {
    auto v = ch5::eval("{\"s\":\"hi\"}");
    EXPECT_EQ(std::string(std::get<ch5::string_view_t>(v["s"])), "hi");
}

TEST(Document, ParseCopyConstruct) {
    std::string json = R"({"user":{"name":"alice","age":42}})";
    document doc(json.c_str());
    EXPECT_FALSE(doc.empty());
    EXPECT_EQ(doc.source().size(), json.size());
    // Accès path absolu
    auto& nameNode = doc["/user/name"];
    EXPECT_TRUE(std::holds_alternative<string_view_t>(nameNode));
    EXPECT_EQ(std::get<string_view_t>(nameNode), "alice");
}

TEST(Document, ParseMoveConstruct) {
    std::string json = R"({"a":1,"b":2})";
    document doc(std::move(json));
    EXPECT_TRUE(std::holds_alternative<object_t>(doc));
    EXPECT_EQ(std::get<number_t>(doc["/b"]).value, "2"); // number_t lazy => imprimé comme texte? (selon mode quick)
}

TEST(Document, EvalOverwritesBufferAndRoot) {
    document doc;
    doc.eval(R"({"x":1})");
    auto firstPtr = doc.source().data();
    doc.eval(R"({"y":2,"very long member name":"very lon value string"})");
    // Buffer remplacé (probablement pointeur différent)
    EXPECT_NE(firstPtr, doc.source().data());
    EXPECT_TRUE(std::holds_alternative<number_t>(doc["/y"]));
}

TEST(Document, RematerializeAnchorsViews) {
    document doc(R"({"root":{"s":"abc","n":123}})");
    // Avant rematerialize
    auto s1 = std::get<string_view_t>(doc["/root/s"]);
    std::string printed = to_string(doc);
    // Forcer une ré-ancrage
    doc.rematerialize();
    // Après rematerialize, les valeurs sont reparsées depuis _buffer
    auto s2 = std::get<string_view_t>(doc["/root/s"]);
    EXPECT_EQ(s2, "abc");
    // L'objet final devrait réimprimer identiquement
    EXPECT_EQ(to_string(doc), printed);
}

TEST(Document, ClearResetsToEmptyObject) {
    document doc(R"({"a":1,"b":2})");
    doc.clear();
    EXPECT_TRUE(std::holds_alternative<object_t>(doc));
    // Objet vide -> impression attendue "{}"
    EXPECT_EQ(to_string(doc), "{}");
}

//TEST(Document, FromFile) {
//    std::string payload = R"({"file":{"ok":true,"val":"z"}})";
//    auto path = make_temp_file(payload);
//    auto doc = document::from_file(path);
//    EXPECT_EQ(std::get<string_view_t>(doc["/file/val"]), "z");
//}

TEST(Document, FromStringMoveSource) {
    std::string src = R"({"moved":true})";
    auto doc = document::from_string(src);
    EXPECT_TRUE(std::holds_alternative<boolean_t>(doc["/moved"]));
    EXPECT_EQ(std::get<boolean_t>(doc["/moved"]), true);
}

TEST(Document, RematerializeWithOptions) {
    document doc(R"({"arr":[1,2,3]})");
    doc["/arr/3"] = 4; // autovivification
    auto before = to_string(doc, R"({"compact":true})");
    doc.rematerialize(cppon{}, Quick);
    auto after = to_string(doc, R"({"compact":true})");
    EXPECT_EQ(before, after);
}

TEST(Document, EvalNullBecomesEmptyObject) {
    document d;
    d.eval((const char*)nullptr);
    EXPECT_EQ(to_string(d), "{}");
}