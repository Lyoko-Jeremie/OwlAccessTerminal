// jeremie

#ifndef OWLACCESSTERMINAL_IMAGESERVICE_H
#define OWLACCESSTERMINAL_IMAGESERVICE_H

#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <boost/asio/executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <boost/array.hpp>
#include "../Log/Log.h"
#include "ImageProtobufDefine/ImageProtocol/ImageProtocol.pb.h"
#include "ImageServiceMail.h"

namespace OwlImageService {

    enum {
        TCP_Receive_Package_Max_Size = (1024 * 1024 * 6)
    }; // 6M

    struct CommonTcpPackage {
        uint32_t size_ = 0;
        boost::asio::streambuf data_{};

// https://stackoverflow.com/questions/7556327/int-char-conversion-in-network-frames-with-c-boostasio
//        // sender
//        // char 4 => uint
//        char char_len[4];
//        char_len[0] = (len >> 0);
//        char_len[1] = (len >> 8);
//        char_len[2] = (len >> 16);
//        char_len[3] = (len >> 24);
//        std::cout << "char[4] len=["
//        << char_len[0] << ',' << char_len[1] << ','
//        << char_len[2] << ',' << char_len[3] << ']'
//        << std::endl;
//        uint32_t uint_len = *(reinterpret_cast<uint32_t *>( char_len ));
//        std::cout << "uint len=" << uint_len << std::endl;
//        // network bytes order
//        uint32_t net_len = htonl( len );
//        std::cout << "net_len=" << net_len << std::endl;

//        // receiver
//        uint32_t net_len;
//        size_t len_len = sock->read_some( boost::asio::buffer( reinterpret_cast<char*>(&net_len), 4), error );
//        uint32_t len = ntohl( net_len );
//        std::cout << "uint len=" << len << std::endl;

        CommonTcpPackage() = default;
    };

    class ImageService;

    class ImageServiceSession : public std::enable_shared_from_this<ImageServiceSession> {
    public:
        explicit ImageServiceSession(
                boost::asio::io_context &ioc,
                boost::asio::ip::tcp::socket &&socket,
                std::weak_ptr<ImageService> &&parents
        ) : ioc_(ioc), socket_(std::move(socket)), parents_(std::move(parents)) {}

//        static void fail(boost::system::error_code ec, const char *what) {
//            BOOST_LOG_OWL(error) << what << ": " << ec.message();
//        }

    private:
        boost::asio::io_context &ioc_;
        boost::asio::ip::tcp::socket socket_;
        std::weak_ptr<ImageService> parents_;

    public:
        void
        start() {
            do_receive_size();
        }

    private:
        void
        close_connect();

    public:
        void
        force_close();

    private:
        void
        do_receive_size();

        void
        do_receive_data(std::shared_ptr<CommonTcpPackage> package_r_);

    private:
        void
        do_send_size(std::shared_ptr<CommonTcpPackage> package_s_);

        void
        do_send_data(std::shared_ptr<CommonTcpPackage> package_s_);

    private:
        void
        do_process_request(std::string &&s);

    };

    class ImageService : public std::enable_shared_from_this<ImageService> {
    public:
        ImageService(
                boost::asio::io_context &ioc,
                const boost::asio::ip::tcp::endpoint &endpoint,
                OwlMailDefine::ServiceCameraMailbox &&mailbox
        );

        ~ImageService() {
            BOOST_LOG_OWL(trace_dtor) << "~ImageService()";
            mailbox_->receiveB2A(nullptr);
        }

        static void fail(boost::system::error_code ec, const char *what) {
            BOOST_LOG_OWL(error) << what << ": " << ec.message();
        }

    private:
        boost::asio::io_context &ioc_;
        boost::asio::ip::tcp::acceptor acceptor_;
        OwlMailDefine::ServiceCameraMailbox mailbox_;

    public:
        // Start accepting incoming connections
        void
        start() {
            do_accept();
        }

    private:
        friend ImageServiceSession;

        void receiveMail(OwlMailDefine::MailCamera2Service &&data) {
            // get callback from data and call it to send back image result
            data->runner(data);
        }

        void sendMail(OwlMailDefine::MailService2Camera &&data) {
            // send cmd to camera
            mailbox_->sendA2B(std::move(data));
        }

    private:
        void
        do_accept();

        void
        on_accept(boost::asio::ip::tcp::socket &&socket);

    };

} // OwlImageService

#endif //OWLACCESSTERMINAL_IMAGESERVICE_H
