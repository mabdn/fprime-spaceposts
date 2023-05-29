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

#include "SpacePosts/Moderator/ModeratorComponentAc.hpp"
#include "ModerationStrategy.hpp"

namespace SpacePosts
{

  class Moderator : public ModeratorComponentBase
  {

    PRIVATE :

        //! The moderation strategy to use to decide whether to forward or discard a SpacePost
        //!
        //! The components uses the strategy design pattern. An object which implements the `ModerationStrategy`
        //! strategy interface can be injected into the component in its constructor. The `ModerationStrategy`
        //! interface requires a single method, which has the signature `bool checkMessage(SpacePost message)`.
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
                                                            or discard a SpacePost*/
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
        SpacePosts::MessageStorageStatus
        moderateMessage_handler(
            const NATIVE_INT_TYPE portNum, /*!< The port number*/
            const SpacePosts::SpacePost &data    /*!<
           the SpacePost to store
           */
        );
  };

} // end namespace SpacePosts

#endif
