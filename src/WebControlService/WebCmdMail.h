// jeremie

#ifndef OWLACCESSTERMINAL_WEBCMDMAIL_H
#define OWLACCESSTERMINAL_WEBCMDMAIL_H

#include <memory>
#include "../AsyncCallbackMailbox/AsyncCallbackMailbox.h"

namespace OwlMailDefine {

    enum class WifiCmd {
        ignore,
        ap,
        scan,
        connect,
    };

    struct Cmd2Web;
    struct Web2Cmd {

        WifiCmd cmd;
        bool enableAp = false;
        std::string connect2SSID;

        // Cmd2Web.runner = Web2Cmd.callbackRunner
        std::function<void(std::shared_ptr<Cmd2Web>)> callbackRunner;
    };
    struct Cmd2Web {
        std::function<void(std::shared_ptr<Cmd2Web>)> runner;
        bool ok = false;
    };

    using WebCmdMailbox =
            std::shared_ptr<
                    OwlAsyncCallbackMailbox::AsyncCallbackMailbox<
                            Web2Cmd,
                            Cmd2Web
                    >
            >;

    using MailWeb2Cmd = WebCmdMailbox::element_type::A2B_t;
    using MailCmd2Web = WebCmdMailbox::element_type::B2A_t;

}

#endif //OWLACCESSTERMINAL_WEBCMDMAIL_H
