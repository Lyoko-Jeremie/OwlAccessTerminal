// jeremie

#ifndef OWLACCESSTERMINAL_EMBEDWEBSERVERTOOLS_H
#define OWLACCESSTERMINAL_EMBEDWEBSERVERTOOLS_H

// https://www.boost.org/doc/libs/develop/libs/beast/example/http/server/async/http_server_async.cpp

#include "../MemoryBoost.h"
#include <string>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <boost/asio/executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace OwlEmbedWebServer {

    // Report a failure
    void
    fail(boost::beast::error_code ec, char const *what);

    // Return a reasonable mime type based on the extension of a file.
    boost::beast::string_view
    mime_type(boost::beast::string_view path);

    // Append an HTTP rel-path to a local filesystem path.
    // The returned path is normalized for the platform.
    std::string
    path_cat(
            boost::beast::string_view base,
            boost::beast::string_view path);

    // This function produces an HTTP response for the given
    // request. The type of the response object depends on the
    // contents of the request, so the interface requires the
    // caller to pass a generic lambda for receiving the response.
    template<
            class Body,
            class Allocator=std::allocator<char>,
            class Send
    >
    void
    handle_request(
            boost::beast::string_view doc_root,
            boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req,
            Send &&send,
            boost::shared_ptr<std::string const> index_file_of_root,
            const std::vector<std::string> allowFileExtList) {
        // Returns a bad request response
        auto const bad_request =
                [&req](boost::beast::string_view why) {
                    boost::beast::http::response<boost::beast::http::string_body> res{
                            boost::beast::http::status::bad_request,
                            req.version()};
                    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
                    res.set(boost::beast::http::field::content_type, "text/html");
                    res.keep_alive(req.keep_alive());
                    res.body() = std::string(why);
                    res.prepare_payload();
                    return res;
                };

        // Returns a not found response
        auto const not_found =
                [&req](boost::beast::string_view target) {
                    boost::beast::http::response<boost::beast::http::string_body> res{
                            boost::beast::http::status::not_found,
                            req.version()};
                    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
                    res.set(boost::beast::http::field::content_type, "text/html");
                    res.keep_alive(req.keep_alive());
                    res.body() = "The resource '" + std::string(target) + "' was not found.";
                    res.prepare_payload();
                    return res;
                };

        // Returns a server error response
        auto const server_error =
                [&req](boost::beast::string_view what) {
                    boost::beast::http::response<boost::beast::http::string_body> res{
                            boost::beast::http::status::internal_server_error,
                            req.version()};
                    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
                    res.set(boost::beast::http::field::content_type, "text/html");
                    res.keep_alive(req.keep_alive());
                    res.body() = "An error occurred: '" + std::string(what) + "'";
                    res.prepare_payload();
                    return res;
                };

        // Make sure we can handle the method
        if (req.method() != boost::beast::http::verb::get &&
            req.method() != boost::beast::http::verb::head)
            return send(bad_request("Unknown HTTP-method"));

        // Request path must be absolute and not contain "..".
        if (req.target().empty() ||
            req.target()[0] != '/' ||
            req.target().find("..") != boost::beast::string_view::npos)
            return send(bad_request("Illegal request-target"));

        // ------------------------ path check --------------------

        std::string reqString{req.target()};
        if (reqString.find("?") != std::string::npos) {
            reqString = reqString.substr(0, reqString.find("?"));
        }
        // Build the path to the requested file
        std::string path = path_cat(doc_root, reqString);
        if (reqString.back() == '/')
            path.append(*index_file_of_root);

        // ------------------------ path check --------------------
        try {
            std::filesystem::path root_path{std::string{doc_root}};
            std::error_code errorCode;
            root_path = std::filesystem::canonical(root_path, errorCode);
            if (errorCode) {
                return send(bad_request("Illegal request-target 2.1"));
            }

            std::filesystem::path file_path{path};
            errorCode.clear();
            file_path = std::filesystem::canonical(file_path, errorCode);
            if (errorCode) {
                return send(bad_request("Illegal request-target 2.2"));
            }

            // from https://stackoverflow.com/a/51436012/3548568
            std::string relCheckString = std::filesystem::relative(file_path, root_path).generic_string();
            if (relCheckString.find("..") != std::string::npos) {
                return send(bad_request("Illegal request-target 2"));
            }

            errorCode.clear();
            if (std::filesystem::is_symlink(std::filesystem::symlink_status(file_path, errorCode))) {
                return send(bad_request("Illegal request-target 3"));
            }
            if (errorCode) {
                return send(bad_request("Illegal request-target 2.3"));
            }

            auto ext = file_path.extension().string();
            if (ext.front() == '.') {
                ext.erase(ext.begin());
            }
            bool isAllow = false;
            for (const auto &a: allowFileExtList) {
                if (ext == a) {
                    isAllow = true;
                    break;
                }
            }
            if (!isAllow) {
                return send(bad_request("Illegal request-target 2.4"));
            }
        } catch (const std::exception &e) {
            boost::ignore_unused(e);
            return send(bad_request("Illegal request-target 4"));
        }
        // ------------------------ path check --------------------

        // Attempt to open the file
        boost::beast::error_code ec;
        boost::beast::http::file_body::value_type body;
        body.open(path.c_str(), boost::beast::file_mode::scan, ec);

        // Handle the case where the file doesn't exist
        if (ec == boost::beast::errc::no_such_file_or_directory)
            return send(not_found(req.target()));

        // Handle an unknown error
        if (ec)
            return send(server_error(ec.message()));

        // Cache the size since we need it after the move
        auto const size = body.size();

        // Respond to HEAD request
        if (req.method() == boost::beast::http::verb::head) {
            boost::beast::http::response<boost::beast::http::empty_body> res{
                    boost::beast::http::status::ok,
                    req.version()};
            res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(boost::beast::http::field::content_type, mime_type(path));
            res.content_length(size);
            res.keep_alive(req.keep_alive());
            return send(std::move(res));
        }

        // Respond to GET request
        boost::beast::http::response<boost::beast::http::file_body> res{
                std::piecewise_construct,
                std::make_tuple(std::move(body)),
                std::make_tuple(boost::beast::http::status::ok, req.version())};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }


}

#endif //OWLACCESSTERMINAL_EMBEDWEBSERVERTOOLS_H
