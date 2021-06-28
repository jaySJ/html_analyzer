#pragma once
#include <string>
#include <tuple>

std::tuple<uint64_t, uint64_t, uint64_t, std::string> getCleanDomTree(const std::string & rawHtml);
std::tuple<uint64_t, uint64_t, uint64_t> parseHtml(const std::string & html);
