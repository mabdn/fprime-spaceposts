// ======================================================================
// \title  Moderator.cpp
// \author marius
// \brief  cpp file for Moderator component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#include <Ref/Moderator/Moderator.hpp>
#include "Fw/Types/BasicTypes.hpp"

#include "ModerationStrategy.hpp"

namespace Ref
{

  // ----------------------------------------------------------------------
  // Construction, initialization, and destruction
  // ----------------------------------------------------------------------

  Moderator ::
      Moderator(
          const char *const compName,
          ModerationStrategy &moderationStrategy) : ModeratorComponentBase(compName),
                                                          m_moderationStrategy(moderationStrategy) // TODO Does this copy?
  {
  }

  void Moderator ::
      init(
          const NATIVE_INT_TYPE instance)
  {
    ModeratorComponentBase::init(instance);
  }

  Moderator ::
      ~Moderator()
  {
  }

  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  Ref::BBSMessageStorageStatus Moderator ::
      moderateMessage_handler(
          const NATIVE_INT_TYPE portNum,
          const Ref::BBSMessage &data)
  {
    if (this->m_moderationStrategy.checkMessage(data))
    {
      return this->acceptedMessage_out(0, data);
    }
    else
    {
      this->log_ACTIVITY_HI_MESSAGE_REJECTED();

      // Return no error so that the behavior for a component using the input port is the same no matter whether
      // a Moderator is used inbetween two components' BBSMessageSet ports (e.g. Transceiver and MessageStorage)
      // or not.
      return Ref::BBSMessageStorageStatus::OK;
    }
  }

} // end namespace Ref
