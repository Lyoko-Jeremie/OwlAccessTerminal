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

    std::shared_ptr<CmdExecuteItem> CmdExecute::createCEI(const std::string &pName, const std::string &params) {
        auto p = std::make_shared<CmdExecuteItem>(
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


} // OwlCmdExecute