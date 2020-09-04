#pragma once

#include "Omnia/UI/Dialog.h"

namespace Omnia {

class WinDialog: public Dialog {
public:
    WinDialog() = default;
    virtual ~WinDialog() = default;

    string OpenFile(const string &filter = "All\0*.*\0") const override;
    string SaveFile(const string &filter = "All\0*.*\0") const override;
};

}
