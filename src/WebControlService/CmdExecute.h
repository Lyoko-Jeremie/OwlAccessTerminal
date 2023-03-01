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
#include "../Log/Log.h"

#include "WebCmdMail.h"
#include "../ConfigLoader/ConfigLoader.h"

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
                std::shared_ptr<OwlConfigLoader::ConfigLoader> &&config,
                OwlMailDefine::WebCmdMailbox &&mailbox
        ) : ioc_(ioc), mailbox_(std::move(mailbox)),
            config_(std::move(config)) {
            mailbox_->receiveA2B([this](OwlMailDefine::MailWeb2Cmd &&data) {
                receiveMail(std::move(data));
            });
        }

        ~CmdExecute() {
            mailbox_->receiveA2B(nullptr);
        }

    private:
        boost::asio::io_context &ioc_;
        OwlMailDefine::WebCmdMailbox mailbox_;

        std::shared_ptr<OwlConfigLoader::ConfigLoader> config_;

        std::atomic_size_t ceiIdGenerator_{1};
        std::mutex ceiMtx_;
        std::unordered_map<size_t, std::weak_ptr<CmdExecuteItem>> ceiPool_;

    public:

    private:
        friend void CmdExecuteItem::destroy() const;

        void removeCEI(size_t CmdExecuteItemId);

        void triggerCleanCeiPool(
                // we force caller pass the lg ref to make sure ceiMtx_ be locked in caller side,
                // so we don't need to lock it again , use this way to avoid double-lock issue
                const std::lock_guard<typeof(ceiMtx_)> &lg
        );

        void sendBackResult(std::shared_ptr<CmdExecuteItem> cei, OwlMailDefine::MailWeb2Cmd data);

        void receiveMail(OwlMailDefine::MailWeb2Cmd &&data);

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
