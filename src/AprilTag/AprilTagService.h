// jeremie

#ifndef OWLACCESSTERMINAL_APRILTAGSERVICE_H
#define OWLACCESSTERMINAL_APRILTAGSERVICE_H

#include <memory>
#include <boost/asio.hpp>
#include <opencv2/opencv.hpp>
#include "../ImageService/ImageServiceMail.h"
#include "../CommandService/CmdSerialMail.h"

namespace OwlAprilTagService {

    struct AprilTagData;

    class AprilTagService : public std::enable_shared_from_this<AprilTagService> {
    public:
        AprilTagService(
                boost::asio::io_context &ioc,
                long msStartTimer,
                long msDurationTimer,
                OwlMailDefine::CmdSerialMailbox &&mailbox_cmd,
                OwlMailDefine::ServiceCameraMailbox &&mailbox_camera
        ) : ioc_(ioc), mailbox_cmd_(mailbox_cmd), mailbox_camera_(mailbox_camera),
            timer_(ioc_, boost::asio::chrono::milliseconds(msStartTimer)),
            msStartTimer_(msStartTimer),
            msDurationTimer_(msDurationTimer) {

            mailbox_camera_->receiveB2A = [this](OwlMailDefine::MailCamera2Service &&data) {
                receiveMailCamera(std::move(data));
            };
            mailbox_cmd_->receiveB2A = [this](OwlMailDefine::MailSerial2Cmd &&data) {
                receiveMailCmd(std::move(data));
            };

            timer_.async_wait(
                    [this, self = shared_from_this()]
                            (const boost::system::error_code &ec) {
                        time_loop(ec);
                    }
            );
        }

    private:
        boost::asio::io_context &ioc_;
        OwlMailDefine::CmdSerialMailbox mailbox_cmd_;
        OwlMailDefine::ServiceCameraMailbox mailbox_camera_;

        boost::asio::steady_timer timer_;

        long msStartTimer_ = 1000;
        long msDurationTimer_ = 300;

        std::shared_ptr<AprilTagData> AprilTagData_{};
    public:

        void receiveMailCamera(OwlMailDefine::MailCamera2Service &&data) {
            // get callback from data and call it to send back image result
            data->runner(data);
        }

        void sendMailCamera(OwlMailDefine::MailService2Camera &&data) {
            // send cmd to camera
            mailbox_camera_->sendA2B(std::move(data));
        }

        void receiveMailCmd(OwlMailDefine::MailSerial2Cmd &&data) {
            // get callback from data and call it to send back image result
            data->runner(data);
        }

        void sendMailCmd(OwlMailDefine::MailCmd2Serial &&data) {
            // send cmd to camera
            mailbox_cmd_->sendA2B(std::move(data));
        }

    private:

        void calcTag(const cv::Mat &image, const std::function<void(void)> &whenEnd);

    private:
        void next_loop(bool frameMode = false) {
            // https://www.boost.org/doc/libs/1_81_0/doc/html/boost_asio/tutorial/tuttimer3.html
            if (frameMode) {
                timer_.expires_at(timer_.expiry() + boost::asio::chrono::milliseconds(msDurationTimer_));
            } else {
                timer_.expires_from_now(boost::asio::chrono::milliseconds(msDurationTimer_));
            };

            //  // time calc
            //  // (p+T+5)<=(now)
            //  if ((timer_.expiry() + boost::asio::chrono::milliseconds(msTimer_ + 3)
            //       - boost::asio::chrono::milliseconds(3))
            //      <= std::chrono::steady_clock::now()) {
            //      timer_.expires_at(timer_.expiry() + boost::asio::chrono::milliseconds(msTimer_));
            //  } else {
            //      // (p+T+5)>(now)
            //      timer_.expires_at(std::chrono::steady_clock::now() + boost::asio::chrono::milliseconds(30));
            //  }

            timer_.async_wait(
                    [this, self = shared_from_this()]
                            (const boost::system::error_code &ec) {
                        time_loop(ec);
                    }
            );
        }

        void time_loop(const boost::system::error_code &ec) {
            if (ec) {
                BOOST_LOG_TRIVIAL(error) << "AprilTagService time_loop ec: " << ec.what();
                // ignore
                return;
            }

            // request image and calc it
            auto m = std::make_shared<OwlMailDefine::Service2Camera>();
            m->camera_id = 2;
            m->callbackRunner = [this, self = shared_from_this()]
                    (const OwlMailDefine::MailCamera2Service &data) {
                // calc it
                boost::asio::dispatch(ioc_, [this, self = shared_from_this(), data]() {
                    this->calcTag(data->image, [this, self = shared_from_this()]() {
                        next_loop(false);
                    });
                });
            };
            sendMailCamera(std::move(m));

            // next_loop(true);

        }

    };

} // OwlAprilTagService

#endif //OWLACCESSTERMINAL_APRILTAGSERVICE_H
