// jeremie

#include "ImageService.h"

#include <opencv2/opencv.hpp>

namespace OwlImageService {
    void ImageServiceSession::close_connect() {

        if (socket_.is_open()) {
            boost::system::error_code ec;
            socket_.shutdown(socket_.shutdown_both, ec);
            ec.clear();
            socket_.cancel(ec);
            ec.clear();
            socket_.close(ec);
        }

    }

    void ImageServiceSession::force_close() {
        boost::asio::post(ioc_, [this, self = shared_from_this()]() {
            close_connect();
        });
    }

    void ImageServiceSession::do_receive_size() {

        if (!socket_.is_open()) {
            // is closed by other function, stop!
            BOOST_LOG_TRIVIAL(info) << "Connection closed by other function, stop!";
            return;
        }

        auto package_r_ = std::make_shared<CommonTcpPackage>();
        boost::asio::async_read(
                socket_,
                boost::asio::buffer(
                        reinterpret_cast<char *>(&package_r_->size_), 4
                ),
                [this, self = shared_from_this(), package_r_](
                        const boost::system::error_code &ec,
                        std::size_t bytes_transferred) {

                    if (ec == boost::asio::error::eof) {
                        // Connection closed cleanly by peer.
                        BOOST_LOG_TRIVIAL(info) << "Connection closed cleanly by peer.";
                        return;
                    }
                    if (ec) {
                        // Connection error
                        BOOST_LOG_TRIVIAL(error) << "Connection error " << ec.what();
                        return;
                    }
                    if (bytes_transferred != 4) {
                        // bytes_transferred
                        BOOST_LOG_TRIVIAL(error) << "do_receive_size bytes_transferred : " << bytes_transferred;
                        return;
                    }

                    package_r_->size_ = ntohl(package_r_->size_);

                    if (package_r_->size_ > TCP_Receive_Package_Max_Size) {
                        // size_ too big, maybe a wrong size_ byte
                        BOOST_LOG_TRIVIAL(warning)
                            << "(package_r_.size_ > TCP_Receive_Package_Max_Size), size_: " << package_r_->size_;
                        // TODO close the connect
                        return;
                    }

                    do_receive_data(package_r_);


                    return;
                }
        );
    }

    void ImageServiceSession::do_receive_data(std::shared_ptr<CommonTcpPackage> package_r_) {

        if (!socket_.is_open()) {
            // is closed by other function, stop!
            BOOST_LOG_TRIVIAL(info) << "Connection closed by other function, stop!";
            return;
        }

        boost::asio::async_read(
                socket_,
                package_r_->data_,
                boost::asio::transfer_exactly(package_r_->size_),
                [this, self = shared_from_this(), package_r_](
                        const boost::system::error_code &ec,
                        std::size_t bytes_transferred) {

                    if (ec == boost::asio::error::eof) {
                        // Connection closed cleanly by peer.
                        BOOST_LOG_TRIVIAL(info) << "Connection closed cleanly by peer.";
                        return;
                    }
                    if (ec) {
                        // Connection error
                        BOOST_LOG_TRIVIAL(error) << "Connection error " << ec.what();
                        return;
                    }
                    if (bytes_transferred != package_r_->size_) {
                        // bytes_transferred
                        BOOST_LOG_TRIVIAL(error) << "do_receive_data bytes_transferred : " << bytes_transferred;
                        return;
                    }

                    // https://stackoverflow.com/questions/44904295/convert-stdstring-to-boostasiostreambuf
                    std::string s((std::istreambuf_iterator<char>(&package_r_->data_)),
                                  std::istreambuf_iterator<char>());

                    do_process_request(std::move(s));

                    // next read

                    do_receive_size();

                    return;
                }
        );

    }

    void ImageServiceSession::do_send_size(std::shared_ptr<CommonTcpPackage> package_s_) {

        if (!socket_.is_open()) {
            // is closed by other function, stop!
            BOOST_LOG_TRIVIAL(info) << "Connection closed by other function, stop!";
            return;
        }

        boost::asio::async_write(
                socket_,
                boost::asio::buffer(
                        (reinterpret_cast<uint32_t *>( &package_s_->size_ )), 4
                ),
                [this, self = shared_from_this(), package_s_](
                        const boost::system::error_code &ec,
                        std::size_t bytes_transferred) {

                    if (ec == boost::asio::error::eof) {
                        // Connection closed cleanly by peer.
                        BOOST_LOG_TRIVIAL(info) << "Connection closed cleanly by peer.";
                        return;
                    }
                    if (ec) {
                        // Connection error
                        BOOST_LOG_TRIVIAL(error) << "Connection error " << ec.what();
                        return;
                    }
                    if (bytes_transferred != 4) {
                        // bytes_transferred
                        BOOST_LOG_TRIVIAL(error) << "do_send_size bytes_transferred : " << bytes_transferred;
                        return;
                    }

                    do_send_data(package_s_);

                }
        );
    }

    void ImageServiceSession::do_send_data(std::shared_ptr<CommonTcpPackage> package_s_) {

        if (!socket_.is_open()) {
            // is closed by other function, stop!
            BOOST_LOG_TRIVIAL(info) << "Connection closed by other function, stop!";
            return;
        }

        boost::asio::async_write(
                socket_,
                package_s_->data_,
                boost::asio::transfer_exactly(package_s_->size_),
                [this, self = shared_from_this(), package_s_](
                        const boost::system::error_code &ec,
                        std::size_t bytes_transferred) {

                    if (ec == boost::asio::error::eof) {
                        // Connection closed cleanly by peer.
                        BOOST_LOG_TRIVIAL(info) << "Connection closed cleanly by peer.";
                        return;
                    }
                    if (ec) {
                        // Connection error
                        BOOST_LOG_TRIVIAL(error) << "Connection error " << ec.what();
                        return;
                    }
                    if (bytes_transferred != package_s_->size_) {
                        // bytes_transferred
                        BOOST_LOG_TRIVIAL(error) << "do_send_size bytes_transferred : " << bytes_transferred;
                        return;
                    }

                    // all ok
                    BOOST_LOG_TRIVIAL(info) << "do_send_data complete." << ec.what();

                    return;
                }
        );
    }

    void ImageServiceSession::do_process_request(std::string &&s) {

        ImageRequest ir;
        ir.ParseFromString(s);

        BOOST_LOG_TRIVIAL(info) << "do_process_request ImageRequest: " << ir.DebugString();

        try {
            switch (ir.cmd_id()) {
                case 1: {
                    // this is a read camera request
                    if (ir.has_package_id() && ir.has_camera_id()) {
                        // TODO get camera image data on here

                        cv::Mat img{6, 6, CV_8UC3, cv::Scalar{0, 0, 0}};

                        // now create ImageResponse package
                        ImageResponse is;
                        is.set_cmd_id(1);
                        {
                            std::vector<uchar> imageBuffer;
                            cv::imencode("jpg", img, imageBuffer,
                                         {cv::ImwriteFlags::IMWRITE_JPEG_QUALITY, 70});

                            is.set_image_height(img.rows);
                            is.set_image_width(img.cols);
                            is.set_image_pixel_channel(img.channels());
                            is.set_image_format(ImageFormat::IMAGE_FORMAT_JPG);

                            img.release();

                            std::string imageString{imageBuffer.begin(), imageBuffer.end()};
                            imageBuffer.clear();

                            is.set_image_data(imageString);
                            is.set_image_data_size(imageString.size());
                        }

                        // now create send package and send it
                        auto package_s_ = std::make_shared<CommonTcpPackage>();
                        // https://stackoverflow.com/questions/44904295/convert-stdstring-to-boostasiostreambuf
                        std::iostream{&package_s_->data_} << is.SerializeAsString();
                        package_s_->size_ = is.GetCachedSize();
                        do_send_size(package_s_);
                    }
                }
                    return;
                default:
                    BOOST_LOG_TRIVIAL(warning) << "do_process_request switch default: " << ir.DebugString();
                    return;
            }
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "do_process_request catch exception on " << ir.DebugString()
                                     << " e: " << e.what();
            return;
        } catch (...) {
            BOOST_LOG_TRIVIAL(error) << "do_process_request catch unknown exception on " << ir.DebugString();
            return;
        }
    }




    // ===============================================================================




    ImageService::ImageService(
            boost::asio::io_context &ioc,
            const boost::asio::ip::tcp::endpoint &endpoint)
            : ioc_(ioc),
              acceptor_strand_(boost::asio::make_strand(ioc)),
              acceptor_(acceptor_strand_) {

        boost::system::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec) {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if (ec) {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
                boost::asio::socket_base::max_listen_connections, ec);
        if (ec) {
            fail(ec, "listen");
            return;
        }

    }

    void ImageService::do_accept() {
        // The new connection gets its own strand
        acceptor_.async_accept(
                acceptor_strand_,
                [this, self = shared_from_this()](
                        boost::system::error_code ec,
                        boost::asio::ip::tcp::socket socket) {
                    if (ec) {
                        ImageService::fail(ec, "accept");
                    } else {
                        on_accept(std::move(socket));
                    }
                });
    }

    void ImageService::on_accept(boost::asio::ip::tcp::socket &&socket) {
        std::make_shared<ImageServiceSession>(ioc_, std::move(socket))->start();
    }
} // OwlImageService