// jeremie

#include "CmdExecute.h"

#include <utility>

namespace OwlCmdExecute {

    void CmdExecuteItem::destroy() const {
        if (auto p = parentRef.lock()) {
            p->removeCEI(ceiId);
        }
    }

    bool CmdExecuteItem::start(
            std::function<void()> &&_callbackEnd
    ) {
        callbackEnd = std::move(_callbackEnd);
        pFullName = boost::process::search_path(pName);
        if (pFullName.empty()) {
            // invalid
            return false;
        }
        c = boost::process::child{
                pName, params,
                boost::process::std_out > ap_out,
                boost::process::std_err > ap_err,
        };
        isRunning = true;
        boost::asio::async_read(
                ap_out, buf_out,
                [this, self = shared_from_this()]
                        (const boost::system::error_code &ec, std::size_t size) {
                    boost::ignore_unused(size);
                    isRunning = false;
                    if (ec) {
                        isErr = true;
                        // call caller to process the result
                        if (callbackEnd) {
                            callbackEnd();
                            // clear it to remove some ref maybe on the func
                            callbackEnd = nullptr;
                        }
                        return;
                    }
                    result = c.exit_code();

                    isEnd = true;

                    // call caller to process the result
                    if (callbackEnd) {
                        callbackEnd();
                        // clear it to remove some ref maybe on the func
                        callbackEnd = nullptr;
                    }
                });
        boost::asio::async_read(
                ap_err, buf_err,
                [](const boost::system::error_code &ec, std::size_t size) {
                    boost::ignore_unused(ec);
                    boost::ignore_unused(size);
                });

        isValid = true;
        return true;
    }

    boost::shared_ptr<CmdExecuteItem> CmdExecute::createCEI(const std::string &pName, const std::string &params) {
        auto p = boost::make_shared<CmdExecuteItem>(
                ceiIdGenerator_.fetch_add(1),
                ioc_, pName, params, weak_from_this()
        );
        {
            std::lock_guard<typeof(ceiMtx_)> lg{ceiMtx_};
            ceiPool_.insert({p->ceiId, p->weak_from_this()});
        }
        return p;
    }

    void CmdExecute::removeCEI(size_t CmdExecuteItemId) {
        {
            std::lock_guard<typeof(ceiMtx_)> lg{ceiMtx_};
            ceiPool_.erase(CmdExecuteItemId);
            triggerCleanCeiPool(lg);
        }
    }

    void CmdExecute::triggerCleanCeiPool(
            // we force caller pass the lg ref to make sure ceiMtx_ be locked in caller side,
            // so we don't need to lock it again , use this way to avoid double-lock issue
            const std::lock_guard<typeof(ceiMtx_)> &lg
    ) {
        boost::ignore_unused(lg);
        auto it = ceiPool_.begin();
        while (it != ceiPool_.end()) {
            if (it->second.expired()) {
                it = ceiPool_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void CmdExecute::receiveMail(OwlMailDefine::MailWeb2Cmd &&data) {
        switch (data->cmd) {
            case OwlMailDefine::WifiCmd::enable:
                // `nmcli wifi on | cat`
            {
                auto cei = createCEI(
                        config_->config().cmd_bash_path,
                        config_->config().wifiCmd.enable);
                cei->start([this, self = shared_from_this(), cei, data]() {
                    this->sendBackResult(cei, data);
                });
            }
                return;
            case OwlMailDefine::WifiCmd::ap:
                // `nmcli dev wifi hotspot ssid "<SSID>" password "<PWD>" ifname "<DEVICE_NAME>" | cat`
            {
                bool c1 = std::all_of(data->SSID.begin(), data->SSID.end(), [](const char &a) {
                    return ('a' <= a && a <= 'z') || ('A' <= a && a <= 'Z') || (a == '_') || (a == '-');
                });
                bool c2 = std::all_of(data->PASSWORD.begin(), data->PASSWORD.end(), [](const char &a) {
                    return ('a' <= a && a <= 'z') || ('A' <= a && a <= 'Z') || (a == '_') || (a == '-');
                });
                bool c3 = std::all_of(data->DEVICE_NAME.begin(), data->DEVICE_NAME.end(), [](const char &a) {
                    return ('a' <= a && a <= 'z') || ('A' <= a && a <= 'Z') || ('0' <= a && a <= '9');
                });
                if (!c1 || !c2 || !c3 ||
                    !data->DEVICE_NAME.starts_with("wlx") || data->DEVICE_NAME.length() < 3 ||
                    data->SSID.length() < 8 || data->PASSWORD.length() < 1) {
                    BOOST_LOG_OWL(warning)
                        << "(!c1 || !c2 || !c3)";
                    OwlMailDefine::MailCmd2Web m = boost::make_shared<OwlMailDefine::Cmd2Web>();
                    m->ok = false;
                    m->s_err = "ERROR (!c1 || !c2 || !c3) ERROR";
                    m->runner = data->callbackRunner;
                    sendMail(std::move(m));
                    return;
                }
                auto s = std::string{config_->config().wifiCmd.ap};
                boost::replace_all(s, "<BSSID>", data->SSID);
                boost::replace_all(s, "<PWD>", data->PASSWORD);
                boost::replace_all(s, "<DEVICE_NAME>", data->DEVICE_NAME);
                auto cei = createCEI(config_->config().cmd_bash_path, s);
                cei->start([this, self = shared_from_this(), cei, data]() {
                    this->sendBackResult(cei, data);
                });
            }
                return;
            case OwlMailDefine::WifiCmd::connect:
                // `nmcli dev wifi connect "<BSSID>" password "<PWD>" ifname "<DEVICE_NAME>" | cat`
            {
                bool c1 = std::all_of(data->SSID.begin(), data->SSID.end(), [](const char &a) {
                    return ('a' <= a && a <= 'z') || ('A' <= a && a <= 'Z') || (a == '_') || (a == '-');
                });
                bool c2 = std::all_of(data->PASSWORD.begin(), data->PASSWORD.end(), [](const char &a) {
                    return ('a' <= a && a <= 'z') || ('A' <= a && a <= 'Z') || (a == '_') || (a == '-');
                });
                bool c3 = std::all_of(data->DEVICE_NAME.begin(), data->DEVICE_NAME.end(), [](const char &a) {
                    return ('a' <= a && a <= 'z') || ('A' <= a && a <= 'Z') || ('0' <= a && a <= '9');
                });
                if (!c1 || !c2 || !c3 ||
                    !data->DEVICE_NAME.starts_with("wlx") || data->DEVICE_NAME.length() < 3 ||
                    data->SSID.length() < 8 || data->PASSWORD.length() < 1) {
                    BOOST_LOG_OWL(warning)
                        << "(!c1 || !c2 || !c3)";
                    OwlMailDefine::MailCmd2Web m = boost::make_shared<OwlMailDefine::Cmd2Web>();
                    m->ok = false;
                    m->s_err = "ERROR (!c1 || !c2 || !c3) ERROR";
                    m->runner = data->callbackRunner;
                    sendMail(std::move(m));
                    return;
                }
                auto s = std::string{config_->config().wifiCmd.connect};
                boost::replace_all(s, "<BSSID>", data->SSID);
                boost::replace_all(s, "<PWD>", data->PASSWORD);
                boost::replace_all(s, "<DEVICE_NAME>", data->DEVICE_NAME);
                auto cei = createCEI(config_->config().cmd_bash_path, s);
                cei->start([this, self = shared_from_this(), cei, data]() {
                    this->sendBackResult(cei, data);
                });
            }
                return;
            case OwlMailDefine::WifiCmd::scan:
                // `nmcli dev wifi list | cat`
            {
                auto cei = createCEI(config_->config().cmd_bash_path, config_->config().wifiCmd.scan);
                cei->start([this, self = shared_from_this(), cei, data]() {
                    this->sendBackResult(cei, data);
                });
            }
                return;
            case OwlMailDefine::WifiCmd::showHotspotPassword:
                // `nmcli dev wifi show-password ifname "<DEVICE_NAME>" | cat`
            {
                auto cei = createCEI(config_->config().cmd_bash_path, config_->config().wifiCmd.showHotspotPassword);
                cei->start([this, self = shared_from_this(), cei, data]() {
                    this->sendBackResult(cei, data);
                });
            }
                return;
            case OwlMailDefine::WifiCmd::getWlanDeviceState:
                // `nmcli dev wifi list ifname "<DEVICE_NAME>" | cat`
            {
                bool c1 = std::all_of(data->DEVICE_NAME.begin(), data->DEVICE_NAME.end(), [](const char &a) {
                    return ('a' <= a && a <= 'z') || ('A' <= a && a <= 'Z') || ('0' <= a && a <= '9');
                });
                if (!c1 || !data->DEVICE_NAME.starts_with("wlx") || data->DEVICE_NAME.length() < 3) {
                    BOOST_LOG_OWL(warning)
                        << "(!c1 || !data->DEVICE_NAME.starts_with(\"wlx\") || data->DEVICE_NAME.length() < 3)";
                    OwlMailDefine::MailCmd2Web m = boost::make_shared<OwlMailDefine::Cmd2Web>();
                    m->ok = false;
                    m->s_err = "(!c1 || !data->DEVICE_NAME.starts_with(\"wlx\") || data->DEVICE_NAME.length() < 3)";
                    m->runner = data->callbackRunner;
                    sendMail(std::move(m));
                    return;
                }
                auto s = std::string{config_->config().wifiCmd.getWlanDeviceState};
                boost::replace_all(s, "<DEVICE_NAME>", data->DEVICE_NAME);
                auto cei = createCEI(config_->config().cmd_bash_path, s);
                cei->start([this, self = shared_from_this(), cei, data]() {
                    this->sendBackResult(cei, data);
                });
            }
                return;
            case OwlMailDefine::WifiCmd::listWlanDevice:
                // `nmcli dev status | grep " wifi "`
            {
                auto s = std::string{config_->config().wifiCmd.listWlanDevice};
                auto cei = createCEI(config_->config().cmd_bash_path, s);
                cei->start([this, self = shared_from_this(), cei, data]() {
                    this->sendBackResult(cei, data);
                });
            }
                return;
            case OwlMailDefine::WifiCmd::ignore:
            default:
                // invalid
                BOOST_LOG_OWL(error) << "receiveMail MailWeb2Cmd ignore/default.";
                {
                    OwlMailDefine::MailCmd2Web m = boost::make_shared<OwlMailDefine::Cmd2Web>();
                    m->ok = false;
                    m->s_err = "ERROR receiveMail MailWeb2Cmd ignore/default. ERROR";
                    m->runner = data->callbackRunner;
                    sendMail(std::move(m));
                }
                return;
        }
    }

    void CmdExecute::sendBackResult(boost::shared_ptr<CmdExecuteItem> cei, OwlMailDefine::MailWeb2Cmd data) {
        boost::asio::dispatch(ioc_, [this, self = shared_from_this(), cei, data]() {
            OwlMailDefine::MailCmd2Web m = boost::make_shared<OwlMailDefine::Cmd2Web>();
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


} // OwlCmdExecute