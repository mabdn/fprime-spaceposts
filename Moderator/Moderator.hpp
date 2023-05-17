// ======================================================================
// \title  Moderator.hpp
// \author marius
// \brief  hpp file for Moderator component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#ifndef Moderator_HPP
#define Moderator_HPP

#include "Ref/Moderator/ModeratorComponentAc.hpp"
#include "ModerationStrategy.hpp"

namespace Ref
{

  class Moderator : public ModeratorComponentBase
  {

    PRIVATE :

        //! The moderation strategy to use to decide whether to forward or discard a BBS message
        //!
        //! The components uses the strategy design pattern. An object which implements the `ModerationStrategy`
        //! strategy interface can be injected into the component in its constructor. The `ModerationStrategy`
        //! interface requires a single method, which has the signature `bool checkMessage(UserMessage message)`.
        ModerationStrategy &m_moderationStrategy;

    public:
      // ----------------------------------------------------------------------
      // Construction, initialization, and destruction
      // ----------------------------------------------------------------------

      //! Construct object Moderator
      //!
      Moderator(
          const char *const compName,                  /*!< The component name*/
          ModerationStrategy &moderationStrategy /*!< The moderation strategy to use to decide whether to forward
                                                            or discard a BBS message*/
      );

      //! Initialize object Moderator
      //!
      void init(
          const NATIVE_INT_TYPE instance = 0 /*!< The instance number*/
      );

      //! Destroy object Moderator
      //!
      ~Moderator();

    PRIVATE :

        // ----------------------------------------------------------------------
        // Handler implementations for user-defined typed input ports
        // ----------------------------------------------------------------------

        //! Handler implementation for moderateMessage
        //!
        Ref::BBSMessageStorageStatus
        moderateMessage_handler(
            const NATIVE_INT_TYPE portNum, /*!< The port number*/
            const Ref::BBSMessage &data    /*!<
           the BBS message to store
           */
        );
  };

} // end namespace Ref

#endif
