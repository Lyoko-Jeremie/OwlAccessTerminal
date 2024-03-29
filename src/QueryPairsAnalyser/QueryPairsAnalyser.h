// jeremie

#ifndef OWLACCESSTERMINAL_QUERYPAIRSANALYSER_H
#define OWLACCESSTERMINAL_QUERYPAIRSANALYSER_H

#include <map>
#include <string>
#include <regex>
#include <boost/beast.hpp>
#include <boost/algorithm/string.hpp>
#include "../OwlLog/OwlLog.h"

namespace OwlQueryPairsAnalyser {

    struct QueryPairsAnalyser {
        using QueryPairsType = std::multimap<std::string, std::string>;

        QueryPairsType queryPairs;

        explicit QueryPairsAnalyser(
                const boost::string_view &request_target
        ) {
            parse(request_target);
        }

        void parse(const boost::string_view &request_target) {

            // from https://github.com/boostorg/beast/issues/787#issuecomment-376259849
            static const std::regex PARSE_URL{R"((/([^ ?]+)?)?/?\??([^/ ]+\=[^/ ]+)?)",
                                              std::regex_constants::ECMAScript | std::regex_constants::icase};
            std::smatch match;
            auto url = std::string{request_target};
            if (std::regex_match(url, match, PARSE_URL) && match.size() == 4) {
                std::string path = match[1];
                std::string query = match[3];

                std::vector<std::string> queryList;
                boost::split(queryList, query, boost::is_any_of("&"));

                for (const auto &a: queryList) {
                    std::vector<std::string> p;
                    boost::split(p, a, boost::is_any_of("="));
                    if (p.size() == 1) {
                        queryPairs.emplace(p.at(0), "");
                    }
                    if (p.size() == 2) {
                        queryPairs.emplace(p.at(0), p.at(1));
                    }
                    if (p.size() > 2) {
                        std::stringstream ss;
                        for (size_t i = 1; i != p.size(); ++i) {
                            ss << p.at(i);
                        }
                        queryPairs.emplace(p.at(0), ss.str());
                    }
                }

//                BOOST_LOG_OWL(info) << "query:" << query;
//                BOOST_LOG_OWL(info) << "queryList:";
//                for (const auto &a: queryList) {
//                    BOOST_LOG_OWL(info) << "\t" << a;
//                }
//                BOOST_LOG_OWL(info) << "queryPairs:";
//                for (const auto &a: queryPairs) {
//                    BOOST_LOG_OWL(info) << "\t" << a.first << " = " << a.second;
//                }

            }
        }

    }; // OwlQueryPairsAnalyser

}
#endif //OWLACCESSTERMINAL_QUERYPAIRSANALYSER_H
