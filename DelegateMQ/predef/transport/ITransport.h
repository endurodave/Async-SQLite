#ifndef ITRANSPORT_H
#define ITRANSPORT_H

#include "DmqHeader.h"
#include "../../delegate/DelegateOpt.h"

/// @brief DelegateMQ transport interface. 
class ITransport
{
public:
    /// Send data to a remote
    /// @param[in] os Output stream to send.
    /// @param[in] header The header to send.
    /// @return 0 if success.
    virtual int Send(xostringstream& os, const DmqHeader& header) = 0;

    /// Receive data from a remote
    /// @param[out] header Incoming delegate message header.
    /// @return The received incoming data bytes, not including the header.
    virtual xstringstream Receive(DmqHeader& header) = 0;
};

#endif