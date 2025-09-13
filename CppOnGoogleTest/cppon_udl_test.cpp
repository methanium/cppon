#include "pch.h"
#include <cppon/c++on.h>

using namespace ch5;
using namespace ch5::literals;

TEST(UDL, JsonQuickAndFull) {
    auto q = R"({"n":1,"s":"x"})"_json;
    auto f = R"({"n":1,"s":"x"})"_jsonf;
    EXPECT_TRUE(std::holds_alternative<object_t>(q));
    EXPECT_TRUE(std::holds_alternative<object_t>(f));
    // quick: nombre paresseux possible (number_t ou numérique, selon options)
    // full: conversion effectuée
    auto& nq = q["/n"];
    auto& nf = f["/n"];
    EXPECT_NO_THROW((void)get_cast<int>(nq));
    EXPECT_NO_THROW((void)get_strict<int64_t>(f["/n"]));
    EXPECT_EQ(std::get<string_view_t>(q["/s"]), "x");
    EXPECT_EQ(std::get<string_view_t>(f["/s"]), "x");
}

TEST(UDL, OptionsAndToString) {
    auto doc = R"({"a":1,"b":2})"_json;
    auto opts = R"({"pretty":true})"_opts;
    auto out = to_string(doc, opts);
    EXPECT_NE(out.find('\n'), std::string::npos); // pretty = retour à la ligne
}

TEST(UDL, PathAndBlob) {
    cppon doc;
    doc["/img/format"] = "png";
    // base64: blob_string_t
    doc["/img/data64"] = "QUJD"_b64;
    // binaire brut
    doc["/img/data"] = "\x01\x02""ABC"_blob;
    // path_t
    doc["/ref"] = "/img/format"_path;

    EXPECT_TRUE(std::holds_alternative<blob_string_t>(doc["/img/data64"]));
    EXPECT_TRUE(std::holds_alternative<blob_t>(doc["/img/data"]));
    EXPECT_TRUE(std::holds_alternative<path_t>(doc["/ref"]));

    // Sérialiser un sous-arbre (compact par défaut)
    auto json = to_string(doc["/img"]);
    EXPECT_NE(json.find("\"format\""), std::string::npos);
}