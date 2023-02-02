// jeremie

#ifndef OWLACCESSTERMINAL_WEBCMDMAIL_H
#define OWLACCESSTERMINAL_WEBCMDMAIL_H

#include <memory>
#include "../AsyncCallbackMailbox/AsyncCallbackMailbox.h"

namespace OwlMailDefine {

    enum class WifiCmd {
        ignore,
        enable,
        ap,
        scan,
        connect,
        showHotspotPassword,
        listWlanDevice,
        getWlanDeviceState,
    };

    struct Cmd2Web;
    struct Web2Cmd {

        WifiCmd cmd;
        bool enableAp = false;
        std::string SSID;
        std::string PASSWORD;
        std::string DEVICE_NAME;

        // Cmd2Web.runner = Web2Cmd.callbackRunner
        std::function<void(std::shared_ptr<Cmd2Web>)> callbackRunner;
    };
    struct Cmd2Web {
        std::function<void(std::shared_ptr<Cmd2Web>)> runner;
        bool ok = false;

        int result = -1;
        std::string s_out;
        std::string s_err;
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
