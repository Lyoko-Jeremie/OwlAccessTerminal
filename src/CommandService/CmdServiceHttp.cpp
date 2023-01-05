// jeremie

#include "CmdServiceHttp.h"

namespace OwlCommandServiceHttp {

    void CmdServiceHttpConnect::sendMail(OwlMailDefine::MailCmd2Serial &&data) {
        // send cmd to serial
        auto p = getParentRef();
        if (p) {
            p->sendMail(std::move(data));
        }
    }

}