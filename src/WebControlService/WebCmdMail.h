// jeremie

#ifndef OWLACCESSTERMINAL_WEBCMDMAIL_H
#define OWLACCESSTERMINAL_WEBCMDMAIL_H

#include <memory>
#include "../AsyncCallbackMailbox/AsyncCallbackMailbox.h"

namespace OwlMailDefine {

    struct Web2Cmd {

        // Cmd2Web.runner = Web2Cmd.callbackRunner
        std::function<void()> callbackRunner;
    };
    struct Cmd2Web {
        std::function<void()> runner;
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
