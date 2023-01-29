// jeremie

#ifndef OWLACCESSTERMINAL_CMDEXECUTE_H
#define OWLACCESSTERMINAL_CMDEXECUTE_H

#include <memory>
#include <utility>
#include <thread>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/atomic.hpp>
#include <boost/log/trivial.hpp>

#include "WebCmdMail.h"

namespace OwlCmdExecute {
//  https://www.boost.org/doc/libs/1_81_0/doc/html/boost_process/tutorial.html

    class CmdExecute;

    struct CmdExecuteItem : public std::enable_shared_from_this<CmdExecuteItem> {
        size_t ceiId;
        std::string pName;
        std::string params;
        boost::filesystem::path pFullName;
        boost::asio::streambuf buf_out;
        boost::asio::streambuf buf_err;
        boost::process::async_pipe ap_out;
        boost::process::async_pipe ap_err;
        boost::process::child c;
        int result = 0;

        bool isValid = false;
        bool isRunning = false;
        bool isEnd = false;
        bool isErr = false;

        std::weak_ptr<CmdExecute> parentRef;

        std::function<void()> callbackEnd;

        explicit CmdExecuteItem(
                size_t ceiId,
                boost::asio::io_context &ioc,
                std::string pName,
                std::string params,
                std::weak_ptr<CmdExecute> parentRef
        ) : ceiId(ceiId),
            pName(std::move(pName)),
            params(std::move(params)),
            ap_out(ioc), ap_err(ioc),
            parentRef(std::move(parentRef)) {
        }

        bool start(std::function<void()> &&callbackEnd);

        void destroy() const;

        ~CmdExecuteItem() {
            destroy();
        }
    };


    class CmdExecute : public std::enable_shared_from_this<CmdExecute> {
    public:
        explicit CmdExecute(
                boost::asio::io_context &ioc,
                std::string cmd_nmcli_path,
                std::string cmd_bash_path,
                OwlMailDefine::WebCmdMailbox &&mailbox
        ) : ioc_(ioc), mailbox_(std::move(mailbox)),
            cmd_nmcli_path_(std::move(cmd_nmcli_path)), cmd_bash_path_(std::move(cmd_bash_path)) {
            mailbox_->receiveA2B = [this](OwlMailDefine::MailWeb2Cmd &&data) {
                receiveMail(std::move(data));
            };
        }

        ~CmdExecute() {
            mailbox_->receiveA2B = nullptr;
        }

    private:
        boost::asio::io_context &ioc_;
        OwlMailDefine::WebCmdMailbox mailbox_;

        std::string cmd_nmcli_path_;
        std::string cmd_bash_path_;

        std::atomic_size_t ceiIdGenerator_{1};
        std::mutex ceiMtx_;
        std::unordered_map<size_t, std::weak_ptr<CmdExecuteItem>> ceiPool_;

    public:
        auto test_(const std::string &pName, const std::string &params) {

            auto p = createCEI(pName, params);

            p->start([p]() {

            });

            return p;
        }

    private:
        friend void CmdExecuteItem::destroy() const;

        void removeCEI(size_t CmdExecuteItemId);

        void triggerCleanCeiPool(
                // we force caller pass the lg ref to make sure ceiMtx_ be locked in caller side,
                // so we don't need to lock it again , use this way to avoid double-lock issue
                const std::lock_guard<typeof(ceiMtx_)> &lg
        );

        void sendBackResult(std::shared_ptr<CmdExecuteItem> cei, OwlMailDefine::MailWeb2Cmd data) {
            boost::asio::dispatch(ioc_, [this, self = shared_from_this(), cei, data]() {
                OwlMailDefine::MailCmd2Web m = std::make_shared<OwlMailDefine::Cmd2Web>();
                m->runner = data->callbackRunner;
                if (cei->isErr) {
                    m->ok = false;
                    sendMail(std::move(m));
                    return;
                }
                if (!cei->isEnd || !cei->isValid) {
                    m->ok = false;
                    sendMail(std::move(m));
                    return;
                }
                m->result = cei->result;
                m->s_err = std::string{
                        (std::istreambuf_iterator<char>(&cei->buf_err)),
                        std::istreambuf_iterator<char>()
                };
                m->s_out = std::string{
                        (std::istreambuf_iterator<char>(&cei->buf_out)),
                        std::istreambuf_iterator<char>()
                };
                m->ok = true;
                sendMail(std::move(m));
            });
        }

        void receiveMail(OwlMailDefine::MailWeb2Cmd &&data) {
            switch (data->cmd) {
                case OwlMailDefine::WifiCmd::enable:
                    // `nmcli wifi on`
                {
                    auto cei = createCEI(cmd_bash_path_, R"(nmcli wifi on)");
                    cei->start([this, self = shared_from_this(), cei, data]() {
                        this->sendBackResult(cei, data);
                    });
                }
                    return;
                case OwlMailDefine::WifiCmd::ap:
                    // `nmcli dev wifi hotspot ssid "<SSID>" password "<PWD>"`
                {
                    bool c1 = std::all_of(data->SSID.begin(), data->SSID.end(), [](const char &a) {
                        return ('a' <= a && a <= 'z') || ('A' <= a && a <= 'Z') || (a == '_') || (a == '-');
                    });
                    bool c2 = std::all_of(data->PASSWORD.begin(), data->PASSWORD.end(), [](const char &a) {
                        return ('a' <= a && a <= 'z') || ('A' <= a && a <= 'Z') || (a == '_') || (a == '-');
                    });
                    if (!c1 || !c2 || data->SSID.length() < 8 || data->PASSWORD.length() < 1) {
                        BOOST_LOG_TRIVIAL(warning)
                            << "(!c1 || !c2 || data->SSID.length() < 8 || data->PASSWORD.length() < 1)";
                        OwlMailDefine::MailCmd2Web m = std::make_shared<OwlMailDefine::Cmd2Web>();
                        m->ok = false;
                        m->s_err = "ERROR (!c1 || !c2 || data->SSID.length() < 8 || data->PASSWORD.length() < 1) ERROR";
                        m->runner = data->callbackRunner;
                        sendMail(std::move(m));
                        return;
                    }
                    auto cei = createCEI(cmd_bash_path_,
                                         std::string{R"(nmcli dev wifi hotspot ssid ")"} +
                                         data->SSID +
                                         std::string{R"(" password ")"} +
                                         data->PASSWORD +
                                         std::string{R"(")"}
                    );
                    cei->start([this, self = shared_from_this(), cei, data]() {
                        this->sendBackResult(cei, data);
                    });
                }
                    return;
                case OwlMailDefine::WifiCmd::connect:
                    // `nmcli dev wifi connect "<BSSID>" password "<PWD>"`
                {
                    bool c1 = std::all_of(data->SSID.begin(), data->SSID.end(), [](const char &a) {
                        return ('a' <= a && a <= 'z') || ('A' <= a && a <= 'Z') || (a == '_') || (a == '-');
                    });
                    bool c2 = std::all_of(data->PASSWORD.begin(), data->PASSWORD.end(), [](const char &a) {
                        return ('a' <= a && a <= 'z') || ('A' <= a && a <= 'Z') || (a == '_') || (a == '-');
                    });
                    if (!c1 || !c2 || data->SSID.length() < 8 || data->PASSWORD.length() < 1) {
                        BOOST_LOG_TRIVIAL(warning)
                            << "(!c1 || !c2 || data->SSID.length() < 8 || data->PASSWORD.length() < 1)";
                        OwlMailDefine::MailCmd2Web m = std::make_shared<OwlMailDefine::Cmd2Web>();
                        m->ok = false;
                        m->s_err = "ERROR (!c1 || !c2 || data->SSID.length() < 8 || data->PASSWORD.length() < 1) ERROR";
                        m->runner = data->callbackRunner;
                        sendMail(std::move(m));
                        return;
                    }
                    auto cei = createCEI(cmd_bash_path_,
                                         std::string{R"(nmcli dev wifi connect ")"} +
                                         data->SSID +
                                         std::string{R"(" password ")"} +
                                         data->PASSWORD +
                                         std::string{R"(")"}
                    );
                    cei->start([this, self = shared_from_this(), cei, data]() {
                        this->sendBackResult(cei, data);
                    });
                }
                    return;
                case OwlMailDefine::WifiCmd::scan:
                    // `nmcli dev wifi list | cat`
                {
                    auto cei = createCEI(cmd_bash_path_, R"(nmcli dev wifi list | cat)");
                    cei->start([this, self = shared_from_this(), cei, data]() {
                        this->sendBackResult(cei, data);
                    });
                }
                    return;
                case OwlMailDefine::WifiCmd::ignore:
                default:
                    // invalid
                    BOOST_LOG_TRIVIAL(error) << "receiveMail MailWeb2Cmd ignore/default.";
                    {
                        OwlMailDefine::MailCmd2Web m = std::make_shared<OwlMailDefine::Cmd2Web>();
                        m->ok = false;
                        m->s_err = "ERROR receiveMail MailWeb2Cmd ignore/default. ERROR";
                        m->runner = data->callbackRunner;
                        sendMail(std::move(m));
                    }
                    return;
            }
        }

        void sendMail(OwlMailDefine::MailCmd2Web &&data) {
            mailbox_->sendB2A(std::move(data));
        }

    private:

        std::shared_ptr<CmdExecuteItem> createCEI(
                const std::string &pName, const std::string &params
        );


    };

} // OwlCmdExecute

#endif //OWLACCESSTERMINAL_CMDEXECUTE_H
